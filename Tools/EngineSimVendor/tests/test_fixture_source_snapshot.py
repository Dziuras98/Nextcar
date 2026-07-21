#!/usr/bin/env python3
from __future__ import annotations
import json, shutil, subprocess, sys, tempfile, unittest
from pathlib import Path

class FixtureSourceSnapshotTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.repo=Path(__file__).resolve().parents[3]
        cls.verifier=cls.repo/"Tools/EngineSimVendor/verify_fixture_source_snapshot.py"
        cls.index=cls.repo/"Tools/EngineSimVendor/FixtureSourceSnapshot/SNAPSHOT.json"
        cls.source=cls.repo/"Tools/EngineSimVendor/FixtureSourceSnapshot/engine-sim"
    def workspace(self):
        temp=tempfile.TemporaryDirectory(); root=Path(temp.name)
        index=root/"SNAPSHOT.json"; snapshot=root/"engine-sim"
        shutil.copy2(self.index,index);shutil.copytree(self.source,snapshot)
        return temp,index,snapshot
    def run_verifier(self,index=None,snapshot=None):
        cmd=[sys.executable,"-S",str(self.verifier)]
        if index is not None: cmd += ["--index",str(index)]
        if snapshot is not None: cmd += ["--snapshot-root",str(snapshot)]
        return subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False,cwd=self.repo)
    def reject(self,mutate):
        temp,index,snapshot=self.workspace();self.addCleanup(temp.cleanup);mutate(index,snapshot)
        result=self.run_verifier(index,snapshot);self.assertNotEqual(result.returncode,0,result.stdout+result.stderr)
    def write(self,path,obj): path.write_text(json.dumps(obj,ensure_ascii=False,sort_keys=True,indent=2)+"\n",encoding="utf-8",newline="\n")
    def test_default_snapshot_passes(self):
        result=self.run_verifier();self.assertEqual(result.returncode,0,result.stdout+result.stderr);self.assertIn("fixture source snapshot PASS",result.stdout)
    def test_missing_snapshot_file_fails(self): self.reject(lambda i,s:(s/"es/objects/objects.mr").unlink())
    def test_changed_snapshot_bytes_fail(self): self.reject(lambda i,s:(s/"es/objects/objects.mr").write_bytes((s/"es/objects/objects.mr").read_bytes()+b"\n"))
    def test_unexpected_snapshot_file_fails(self):
        def m(i,s): (s/"unexpected.txt").write_text("x",encoding="utf-8")
        self.reject(m)
    def test_path_traversal_fails(self):
        def m(i,s): d=json.loads(i.read_text());d["files"][0]["path"]="../escape";self.write(i,d)
        self.reject(m)
    def test_wrong_commit_fails(self):
        def m(i,s): d=json.loads(i.read_text());d["commit"]="0"*40;self.write(i,d)
        self.reject(m)
    def test_wrong_tree_fails(self):
        def m(i,s): d=json.loads(i.read_text());d["tree"]="0"*40;self.write(i,d)
        self.reject(m)
if __name__=="__main__": unittest.main()
