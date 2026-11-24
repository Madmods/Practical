"""Bounded recovery policy. No keys, process reads, packet writes or downloads."""
from collections import deque

INITIAL_READY_SECONDS = 5.0

class TransportHealth:
    def __init__(self, now):
        self.directions = {direction: dict(last_good=now, failures=0, successes=2)
                           for direction in ('outgoing', 'incoming')}

    def record(self, direction, authenticated, now):
        state = self.directions[direction]
        if authenticated:
            state['last_good'] = now
            state['successes'] += 1
            if state['successes'] >= 2:
                state['failures'] = 0
        else:
            state['successes'] = 0
            state['failures'] += 1

    @property
    def suspect(self):
        return any(state['failures'] for state in self.directions.values())

    def needs_refresh(self, now):
        return any(state['failures'] >= 8 and now - state['last_good'] >= .5
                   for state in self.directions.values())

class RecoveryBudget:
    """Three fresh preparations per two minutes, then require an actual update."""
    def __init__(self):
        self.attempts = deque()

    def next_delay(self, now, reason):
        if reason not in ('authentication', 'connection-changed', 'traffic-stalled'):
            return None
        while self.attempts and now - self.attempts[0] >= 120:
            self.attempts.popleft()
        if len(self.attempts) >= 3:
            return None
        delay = (1., 2., 4.)[len(self.attempts)]
        self.attempts.append(now)
        return delay

def preparation_presentation(phase, elapsed):
    if phase.startswith('keys-') and elapsed >= INITIAL_READY_SECONDS:
        return 'recovering', 'No verified '+phase[5:]+' keys after 5 seconds. Recovering the current connection automatically'
    return phase, None
