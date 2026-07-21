#!/usr/bin/env python3
from __future__ import annotations
import hashlib,json,re,shutil,subprocess,sys,tempfile,unittest
from pathlib import Path
SOURCE_FILES=("assets/engines/atg-video-2/01_subaru_ej25_eh.mr","es/objects/objects.mr","es/sound-library/impulse_responses.mr","scripting/include/engine_node.h")
class SubaruFixtureVerifierTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.repo=Path(__file__).resolve().parents[3];cls.verifier=cls.repo/"Tools/EngineSimVendor/verify_subaru_ej25_fixture.py"
 def make_workspace(self):
  temporary=tempfile.TemporaryDirectory();work=Path(temporary.name)
  src0=self.repo/"Tools/EngineSimVendor/FixtureSourceSnapshot/engine-sim";src=work/"Tools/EngineSimVendor/FixtureSourceSnapshot/engine-sim"
  for rel in SOURCE_FILES:(src/rel).parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src0/rel,src/rel)
  snapshot_index=work/"Tools/EngineSimVendor/FixtureSourceSnapshot/SNAPSHOT.json";snapshot_index.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(self.repo/"Tools/EngineSimVendor/FixtureSourceSnapshot/SNAPSHOT.json",snapshot_index)
  rels={"spec":"Tests/EngineSimCore/Fixtures/subaru_ej25_atg_video_2_fixture.json","fixture_cpp":"Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Fixtures/SubaruEJ25AtgVideo2Fixture.cpp","fixture_header":"Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Fixtures/SubaruEJ25AtgVideo2Fixture.h","fixture_spec":"Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Fixtures/SubaruEJ25AtgVideo2FixtureSpec.h","ir_header":"Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/MinimalMuffling02ImpulseResponse.generated.h"}
  paths={}
  for key,rel in rels.items():
   original=self.repo/rel;copy=work/rel;copy.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(original,copy);paths[key]=copy
  shard_src=(self.repo/rels["spec"]).with_suffix("");shutil.copytree(shard_src,paths["spec"].with_suffix(""));paths["source_root"]=src;paths["snapshot_index"]=snapshot_index
  return temporary,paths
 def run_verifier(self,paths,explicit_source=True):
  command=[sys.executable,"-S",str(self.verifier),"--spec",str(paths["spec"]),"--snapshot-index",str(paths["snapshot_index"]),"--fixture-cpp",str(paths["fixture_cpp"]),"--fixture-header",str(paths["fixture_header"]),"--ir-header",str(paths["ir_header"])]
  if explicit_source:command += ["--source-root",str(paths["source_root"])]
  return subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False,cwd=self.repo)
 def assert_rejected(self,mutate):
  temporary,paths=self.make_workspace();self.addCleanup(temporary.cleanup);mutate(paths);result=self.run_verifier(paths);self.assertNotEqual(result.returncode,0,msg=f"mutation unexpectedly passed\n{result.stdout}\n{result.stderr}")
 @staticmethod
 def replace_once(path,old,new):
  text=path.read_text(encoding="utf-8");count=text.count(old)
  if count!=1:raise AssertionError(f"expected one {old!r}, found {count}")
  path.write_text(text.replace(old,new,1),encoding="utf-8",newline="\n")
 @staticmethod
 def index(paths):return json.loads(paths["spec"].read_text(encoding="utf-8"))
 @staticmethod
 def write(path,obj):path.write_text(json.dumps(obj,ensure_ascii=False,indent=2,sort_keys=True)+"\n",encoding="utf-8",newline="\n")
 @classmethod
 def shard(cls,paths,index=0):return paths["spec"].parent/cls.index(paths)["ordered_shards"][index]["path"]
 def test_default_repository_verifier_passes_without_source_root(self):
  result=subprocess.run([sys.executable,"-S",str(self.verifier)],cwd=self.repo,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False);self.assertEqual(result.returncode,0,result.stdout+result.stderr);self.assertIn("total: 118",result.stdout)
 def test_valid_fixture_passes(self):
  temporary,paths=self.make_workspace();self.addCleanup(temporary.cleanup);result=self.run_verifier(paths);self.assertEqual(result.returncode,0,msg=result.stdout+result.stderr);self.assertIn("fixture source snapshot PASS",result.stdout);self.assertIn("all shard hashes PASS",result.stdout);self.assertIn("semantic contract digest PASS",result.stdout);self.assertIn("total: 118",result.stdout)
 def test_explicit_script_value_change_is_detected(self):self.assert_rejected(lambda p:self.replace_once(p["source_root"]/SOURCE_FILES[0],"starter_torque: 70 * units.lb_ft","starter_torque: 71 * units.lb_ft"))
 def test_library_default_change_is_detected(self):self.assert_rejected(lambda p:self.replace_once(p["source_root"]/"es/objects/objects.mr","input max_dilution_effect [float]: 10.0","input max_dilution_effect [float]: 11.0"))
 def test_source_anchor_change_is_detected(self):self.assert_rejected(lambda p:self.replace_once(p["source_root"]/SOURCE_FILES[0],"starter_torque: 70 * units.lb_ft","starter_torque: 70.0 * units.lb_ft"))
 def test_source_line_context_change_is_detected(self):self.assert_rejected(lambda p:self.replace_once(p["source_root"]/SOURCE_FILES[0],"starter_speed: 500 * units.rpm","starter_speed: 501 * units.rpm"))
 def test_collector_area_change_in_fixture_is_detected(self):self.assert_rejected(lambda p:self.replace_once(p["fixture_spec"],"double ExhaustCollectorAreaSquareInch = 3.14159265358979323846;","double ExhaustCollectorAreaSquareInch = 1.5625;"))
 def test_single_flow_table_element_change_is_detected(self):self.assert_rejected(lambda p:self.replace_once(p["fixture_spec"],"268, 280, 280, 281","268, 280, 280, 282"))
 def test_journal_mapping_change_is_detected(self):self.assert_rejected(lambda p:self.replace_once(p["fixture_spec"],"JournalMapping{{0, 3, 1, 2}}","JournalMapping{{0, 2, 1, 3}}"))
 def test_cam_centerline_change_is_detected(self):self.assert_rejected(lambda p:self.replace_once(p["fixture_spec"],"477.0, 657.0, 837.0, 1017.0","478.0, 657.0, 837.0, 1017.0"))
 def test_firing_order_change_is_detected(self):self.assert_rejected(lambda p:self.replace_once(p["fixture_spec"],"FiringOrderDeg{{0.0, 180.0, 360.0, 540.0}}","FiringOrderDeg{{0.0, 360.0, 180.0, 540.0}}"))
 def test_deterministic_seed_change_is_detected(self):self.assert_rejected(lambda p:self.replace_once(p["fixture_spec"],"0x4E433034u","0x4E433044u"))
 def test_identity_pcm_is_detected(self):
  def mutate(p):
   self.replace_once(p["ir_header"],"MinimalMuffling02SampleCount = 17555","MinimalMuffling02SampleCount = 1");text=p["ir_header"].read_text();match=re.search(r"MinimalMuffling02Pcm\[\]\s*=\s*\{",text);p["ir_header"].write_text(text[:match.end()]+text[match.end():].replace("-32","32767",1),encoding="utf-8",newline="\n")
  self.assert_rejected(mutate)
 def test_runtime_wav_filename_is_detected(self):self.assert_rejected(lambda p:p["fixture_cpp"].write_text(p["fixture_cpp"].read_text()+'\nstatic const char *RuntimeWav="x.wav";\n',encoding="utf-8",newline="\n"))
 def test_vehicle_dependency_is_detected(self):self.assert_rejected(lambda p:p["fixture_header"].write_text('#include "vehicle.h"\n'+p["fixture_header"].read_text(),encoding="utf-8",newline="\n"))
 def test_removed_spec_record_is_detected(self):
  def mutate(p):data=json.loads(self.shard(p,-1).read_text());data["records"].pop();self.write(self.shard(p,-1),data)
  self.assert_rejected(mutate)
 def test_duplicate_field_id_is_detected(self):
  def mutate(p):data=json.loads(self.shard(p).read_text());data["records"].append(dict(data["records"][0]));self.write(self.shard(p),data)
  self.assert_rejected(mutate)
 def test_unknown_category_is_rejected(self):
  def mutate(p):data=json.loads(self.shard(p).read_text());data["records"][0]["category"]="unknown-category";self.write(self.shard(p),data)
  self.assert_rejected(mutate)
 def test_missing_shard_is_rejected(self):self.assert_rejected(lambda p:self.shard(p,2).unlink())
 def test_changed_shard_sha_is_rejected(self):self.assert_rejected(lambda p:self.shard(p,3).write_text(self.shard(p,3).read_text()+" ",encoding="utf-8"))
 def test_duplicate_record_across_shards_is_rejected(self):
  def mutate(p):
   first=json.loads(self.shard(p,0).read_text());second=json.loads(self.shard(p,1).read_text());second["records"][0]=dict(first["records"][0]);self.write(self.shard(p,1),second);index=self.index(p);raw=self.shard(p,1).read_bytes();index["ordered_shards"][1]["sha256"]=hashlib.sha256(raw).hexdigest();self.write(p["spec"],index)
  self.assert_rejected(mutate)
 def test_changed_shard_ordering_is_rejected(self):
  def mutate(p):index=self.index(p);index["ordered_shards"][0],index["ordered_shards"][1]=index["ordered_shards"][1],index["ordered_shards"][0];self.write(p["spec"],index)
  self.assert_rejected(mutate)
 def test_semantic_digest_mismatch_is_rejected(self):
  def mutate(p):index=self.index(p);index["semantic_contract_sha256"]="0"*64;self.write(p["spec"],index)
  self.assert_rejected(mutate)
 def test_path_traversal_in_shard_path_is_rejected(self):
  def mutate(p):index=self.index(p);index["ordered_shards"][0]["path"]="../escape.json";self.write(p["spec"],index)
  self.assert_rejected(mutate)
if __name__=="__main__":unittest.main()
