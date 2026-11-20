"""Cross-check generated native plans against the installed helper's physics.
Run scripts/test-ff3-trajectory.cmd first. No live traffic or process access.
"""
import csv,math,pathlib,sys
native=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(native/'diagnostics/network_decoder'))
from qb_packet_editor import velocity
with (native/'build/joint-dispatch-fixtures.csv').open() as file:
    rows=list(csv.DictReader(file))
assert len(rows)==16
for row in rows:
    q={k:float(v) for k,v in row.items()}
    launch,time=velocity([0,8,0],[q[k] for k in ('tx','ty','tz')],[q[k] for k in ('mx','my','mz')],q['power'],28,.06,bool(q['lob']),5)
    assert abs(time-q['time'])<1e-5,q
    assert math.dist(launch,[q[k] for k in ('vx','vy','vz')])<1e-4,q
print(f'{len(rows)} native/helper trajectory agreement checks passed')
