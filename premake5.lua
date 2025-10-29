PROJECT_GENERATOR_VERSION = 3

newoption({
  trigger = "gmcommon",
  description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
  value = "../garrysmod_common"
})

local gmcommon = assert(_OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON"),
  "you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")

include(gmcommon)

CreateWorkspace({name = "crashdumps", abi_compatible = false, path = "projects/" .. os.target() .. "/" .. _ACTION})

platforms { "x86", "x86_64" }
configurations { "Debug", "Release" }

filter("platforms:x86")
  architecture "x86"
filter("platforms:x86_64")
  architecture "x86_64"

CreateProject({serverside = true, source_path = "source", manual_files = false})
IncludeLuaShared()

filter("system:windows")
  links { "DbgHelp" }
  files { "source/win32/*.cpp", "source/win32/*.hpp" }
