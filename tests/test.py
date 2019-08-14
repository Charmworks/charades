# TODO: Run tests in this directory too?
# TODO: Allow make test from individual model dirs
# TODO: Config files for different levels of testing
# TODO: Config variants? ie with load balancing?
# TODO: Better parsing for test name

import subprocess
from collections import namedtuple
import os
import sys

build_dir = os.path.abspath('../src/build')
model_dir = os.path.abspath('../models')
test_dir = os.path.abspath('.')

charmrun = os.path.join(build_dir,'charmrun')

CfgElem = namedtuple('CfgElem', 'name path')

class CharmConfig:
  def __init__(self, name, args):
    self.name = name
    self.args = args

class TestConfig:
  exe = CfgElem("","")
  model_cfg = CfgElem("","")
  test_cfg = CfgElem("","")
  charm_cfg = ""
  stdout_path = ""
  stderr_path = ""
  status = 0

class ModelConfig:
  def __init__(self, name, root, exe):
    self.name = name
    self.root = root
    self.exe = os.path.join(root, exe)
    cfg_dir = os.path.join(root, 'test_configs')
    self.cfgs = [os.path.join(cfg_dir, c) for c in os.listdir(cfg_dir) if os.path.isfile(os.path.join(cfg_dir, c))]

def runTest(cfg):
  outdir = os.path.join("test_output", os.path.relpath(os.path.dirname(cfg.exe.path),start=model_dir), os.path.basename(cfg.model_cfg.path), os.path.basename(cfg.test_cfg.path), cfg.charm_cfg.name)
  os.makedirs(outdir, exist_ok=True)
  cfg.stdout_path = os.path.join(outdir, "test.out")
  cfg.stderr_path = os.path.join(outdir, "test.err")
  print ("================================")
  print ("Running Test:")
  print ("Executable:\t"+cfg.exe.name, "("+cfg.exe.path+")")
  print ("Charm Config:\t"+str(cfg.charm_cfg.args))
  print ("Model Config:\t"+cfg.model_cfg.name, "("+cfg.model_cfg.path+")")
  print ("Test Config:\t"+cfg.test_cfg.name, "("+cfg.test_cfg.path+")")
  print ("Test STDOUT:\t"+cfg.stdout_path)
  print ("Test STDERR:\t"+cfg.stderr_path)

  try:
    with open(cfg.stdout_path, "w") as fout, open(cfg.stderr_path, "w") as ferr:
      result = subprocess.call([charmrun] + cfg.charm_cfg.args + [cfg.exe.path, cfg.model_cfg.path, cfg.test_cfg.path], stdout=fout, stderr=ferr, timeout=10)
  except subprocess.TimeoutExpired:
    cfg.status = -1
    print("STATUS:\t\tTimeout!")
  else:
    cfg.status = result
    if result == 0:
      print ("STATUS:\t\tSuccess!")
    else:
      print ("STATUS:\t\tFailed!")

phold_config = ModelConfig("Imbalanced PHOLD", os.path.join(model_dir, 'phold/imbalanced'), 'phold')
example_config = ModelConfig("Example Mode", os.path.join(model_dir, 'example'), 'example')

model_configs = [phold_config, example_config]

cfg_path = 'test_configs/'

if len(sys.argv) == 1:
  test_cfgs = [os.path.join(cfg_path,f) for f in os.listdir(cfg_path) if os.path.isfile(os.path.join(cfg_path, f))]
else:
  test_cfgs = [os.path.join(cfg_path,f) for f in set(os.listdir(cfg_path)).intersection(sys.argv[1:]) if os.path.isfile(os.path.join(cfg_path, f))]

charm_cfgs = [CharmConfig("1",['+p1','++local']), CharmConfig("2", ['+p2', '++local'])]

tests = []
for m in model_configs:
  subprocess.call(["make", "clean", "-C", m.root])
  subprocess.call(["make", "-C", m.root])
  for mcfg in m.cfgs:
    for t in test_cfgs:
      for c in charm_cfgs:
        cfg = TestConfig()
        cfg.exe = CfgElem(m.name, m.exe)
        f = open(mcfg, "r")
        name = f.readline()[2:-1]
        f.close()
        cfg.model_cfg = CfgElem(name, mcfg)
        f = open(t, "r")
        name = f.readline()[2:-1]
        f.close()
        cfg.test_cfg = CfgElem(name, t)
        cfg.charm_cfg = c
        tests.append(cfg)

print("Running", len(tests), "tests...")
for t in tests:
  runTest(t)

passed = len([t for t in tests if t.status == 0])
failed = len([t for t in tests if t.status > 0])
timeout = len([t for t in tests if t.status == -1])

print ("================")
print ("================")
print ("Passed:\t\t"+str(passed)+"/"+str(len(tests)))
print ("Failed:\t\t"+str(failed)+"/"+str(len(tests)))
print ("Timed-Out:\t"+str(timeout)+"/"+str(len(tests)))
print ("================")
