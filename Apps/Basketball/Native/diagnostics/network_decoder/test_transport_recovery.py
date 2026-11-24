import json,pathlib,tempfile,unittest
from types import SimpleNamespace
from unittest.mock import Mock,patch
import qb_packet_host as host
from transport_recovery import TransportHealth,RecoveryBudget,preparation_presentation

class RecoveryTests(unittest.TestCase):
    def test_five_second_initial_deadline_never_fakes_readiness(self):
        for phase in ('keys-outgoing','keys-incoming'):
            self.assertEqual(preparation_presentation(phase,4.999),(phase,None))
            state,message=preparation_presentation(phase,5.)
            self.assertEqual(state,'recovering');self.assertIn('No verified',message)
        for phase in ('waiting','checking','ready','rejoin'):
            self.assertEqual(preparation_presentation(phase,20),(phase,None))

    def test_one_bad_packet_pauses_edits_but_does_not_restart(self):
        health=TransportHealth(10);health.record('incoming',False,10.1)
        self.assertTrue(health.suspect);self.assertFalse(health.needs_refresh(11))
        health.record('incoming',True,11);self.assertTrue(health.suspect)
        health.record('incoming',True,11.01);self.assertFalse(health.suspect)

    def test_each_direction_must_authenticate_independently(self):
        for direction in ('outgoing','incoming'):
            health=TransportHealth(10)
            for index in range(8):health.record(direction,False,10.05+index*.07)
            health.record('outgoing' if direction=='incoming' else 'incoming',True,10.7)
            self.assertTrue(health.needs_refresh(10.7))
            self.assertFalse(health.needs_refresh(10.49))

    def test_recovery_is_bounded_and_only_for_refreshable_failures(self):
        budget=RecoveryBudget()
        self.assertEqual([budget.next_delay(t,'authentication') for t in (1,2,3,4)],[1.,2.,4.,None])
        self.assertEqual(budget.next_delay(125,'authentication'),1.)
        for reason in ('schema-missing','read-denied','unsupported-client',''):
            self.assertIsNone(RecoveryBudget().next_delay(1,reason))

    def exercise_wrapper(self,reason,stop=False):
        clock=[1.];calls=[];kernel=SimpleNamespace(GetTickCount64=lambda:int(clock[0]*1000))
        with tempfile.TemporaryDirectory() as directory:
            folder=pathlib.Path(directory)
            def run(session,pid,parent,seconds=None,recovery_attempt=0):
                calls.append(recovery_attempt)
                (session/'state.json').write_text(json.dumps(dict(recoveryReason=reason,capture='Stopped',ready=False,status='fixture',keysWritten=False)))
                if stop:(session/'stop').touch()
                return 1
            with patch.object(host,'run',side_effect=run),patch.object(host.time,'monotonic',side_effect=lambda:clock[0]),patch.object(host.time,'sleep',side_effect=lambda seconds:clock.__setitem__(0,clock[0]+seconds)),patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)):
                self.assertEqual(host.run_recovering(folder,77,0),1)
            return calls,json.loads((folder/'state.json').read_text())

    def test_authentication_failure_automatically_starts_fresh_preparations(self):
        calls,state=self.exercise_wrapper('authentication')
        self.assertEqual(calls,[0,1,2,3]);self.assertEqual(state['phase'],'update-required')
        self.assertTrue(state['updateRequired']);self.assertFalse(state['ready']);self.assertFalse(state['keysWritten'])

    def test_missing_schema_and_read_failures_do_not_rescan_keys(self):
        for reason in ('schema-missing','read-denied',''):
            calls,state=self.exercise_wrapper(reason)
            self.assertEqual(calls,[0]);self.assertFalse(state['ready'])

    def test_user_stop_prevents_an_automatic_restart(self):
        calls,state=self.exercise_wrapper('authentication',stop=True)
        self.assertEqual(calls,[0]);self.assertFalse(state['ready'])

if __name__=='__main__':unittest.main()
