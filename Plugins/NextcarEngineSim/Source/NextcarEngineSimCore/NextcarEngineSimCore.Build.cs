using UnrealBuildTool;
using System;
using System.Diagnostics;
using System.IO;

public class NextcarEngineSimCore : ModuleRules
{
    public NextcarEngineSimCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.Add("Core");

        string ProjectRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../.."));
        string BootstrapMarker = Path.Combine(ProjectRoot, "Tools", "EngineSimVendor", "RUNNER_BOOTSTRAP_REQUEST");
        if (File.Exists(BootstrapMarker))
        {
            string Script = Path.Combine(ProjectRoot, "Tools", "EngineSimVendor", "bootstrap_on_runner.ps1");
            ProcessStartInfo StartInfo = new ProcessStartInfo
            {
                FileName = "powershell.exe",
                Arguments = "-NoProfile -ExecutionPolicy Bypass -File \"" + Script + "\" -ProjectRoot \"" + ProjectRoot + "\"",
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true
            };

            using (Process BootstrapProcess = Process.Start(StartInfo))
            {
                string StandardOutput = BootstrapProcess.StandardOutput.ReadToEnd();
                string StandardError = BootstrapProcess.StandardError.ReadToEnd();
                BootstrapProcess.WaitForExit();
                Log.TraceInformation(StandardOutput);
                if (!String.IsNullOrWhiteSpace(StandardError))
                {
                    Log.TraceWarning(StandardError);
                }
                if (BootstrapProcess.ExitCode != 0)
                {
                    throw new BuildException("NC-003B trusted vendor bootstrap failed with exit code {0}.", BootstrapProcess.ExitCode);
                }
            }
            throw new BuildException("NC-003B trusted vendor bootstrap completed. Intentional stop after artifact creation.");
        }
    }
}
