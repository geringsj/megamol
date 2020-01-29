-- Standard MegaMol Configuration File --

print("I am the Standard MegaMol configuration!")

basePath = "D:/Forschungsprojekt/MegamolNeu/"

mmSetLogLevel(0)
mmSetEchoLevel("*")
mmSetAppDir(basePath .. "bin")
mmAddShaderDir(basePath .. "share/shaders")
mmAddResourceDir(basePath .. "share/shaders")
mmAddResourceDir(basePath .. "share/resources")
mmPluginLoaderInfo(basePath .. "bin", "*.mmplg", "include")

computer = mmGetMachineName()

mmSetConfigValue("*-window",    "x5y35w1280h720")
mmSetConfigValue("consolegui",  "on")
mmSetConfigValue("topmost",     "off")
mmSetConfigValue("fullscreen",  "off")
mmSetConfigValue("vsync",       "off")
mmSetConfigValue("useKHRdebug", "off")
mmSetConfigValue("arcball",     "off")

