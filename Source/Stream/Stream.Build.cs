// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;

public class Stream : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }
    public Stream(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Http",
            "Json",
            "JsonUtilities",
            "HeadMountedDisplay",
            "Sockets",
            "Networking"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        string LibrariesPath = Path.Combine(ThirdPartyPath, "ffmpeg", "lib");
        string DLLPath = Path.Combine(ThirdPartyPath, "ffmpeg", "dll");
        string IncludesPath = Path.Combine(ThirdPartyPath, "ffmpeg", "include");

        Console.WriteLine(LibrariesPath);
        Console.WriteLine(DLLPath);
        Console.WriteLine(IncludesPath);

        PrivateIncludePaths.Add(IncludesPath);

        PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avcodec.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avdevice.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avfilter.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avformat.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avutil.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "postproc.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "swresample.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "swscale.lib"));
    }
}
