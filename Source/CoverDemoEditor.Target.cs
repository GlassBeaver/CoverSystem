// Copyright (c) 2018 David Nadaski. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class CoverDemoEditorTarget : TargetRules
{
	public CoverDemoEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		//DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "CoverDemo" } );
	}
}
