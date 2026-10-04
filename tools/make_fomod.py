"""Builds the Gourmet - AutoPatch FOMOD installer (package/Gourmet - AutoPatch - Installer[.zip]).

Two builds of the same plugin ship in one package; the installer picks the one that fits the game version (gameDependency >= 1.6.1170.0 -> "new",
anything older, VR included -> "older") and always lets the user change the choice.
  new   = built on alandtse's CommonLibSSE-NG   (ng-build/build/release/out/GourmetAutoPatch)
  older = built on CharmedBaryon's CommonLibSSE-NG (plugin/build/release)
Build both first (plugin/build.cmd and ng-build/build.cmd GourmetAutoPatch).
Usage: python tools/make_fomod.py [version]
"""
import os
import shutil
import sys
import xml.dom.minidom
import zipfile

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSION = sys.argv[1] if len(sys.argv) > 1 else "1.0.0"
DEST = os.path.join(HERE, "package", "Gourmet - AutoPatch - Installer")
DLL = "GourmetAutoPatch.dll"
SOURCES = {
    "new": os.path.join("E:\\", "WorkSpace", "ng-build", "build", "release", "out", "GourmetAutoPatch", DLL),
    "older": os.path.join(HERE, "plugin", "build", "release", DLL),
}
for k, p in SOURCES.items():
    if not os.path.isfile(p):
        raise SystemExit(f"missing {k} build: {p}")
if os.path.exists(DEST):
    shutil.rmtree(DEST)
for k, p in SOURCES.items():
    d = os.path.join(DEST, k, "SKSE", "Plugins")
    os.makedirs(d)
    shutil.copyfile(p, os.path.join(d, DLL))
shutil.copytree(os.path.join(HERE, "assets", "Interface"), os.path.join(DEST, "common", "Interface"))
os.makedirs(os.path.join(DEST, "fomod"))

NEW_VER = "1.6.1170.0"
config = f"""<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">
  <moduleName>Gourmet - AutoPatch</moduleName>
  <requiredInstallFiles>
    <folder source="common" destination=""/>
    <file source="README.txt" destination="Gourmet - AutoPatch README.txt"/>
    <file source="LICENSE.txt" destination="Gourmet - AutoPatch LICENSE.txt"/>
  </requiredInstallFiles>
  <installSteps order="Explicit">
    <installStep name="Game version">
      <optionalFileGroups order="Explicit">
        <group name="Which build do you want?" type="SelectExactlyOne">
          <plugins order="Explicit">
            <plugin name="Skyrim 1.6.1170 and newer (SE / AE)">
              <description>The new build. Works on Skyrim SE/AE 1.6.1170 and on every later version, including 1.7.x. Pre-selected when the installer sees a game version of 1.6.1170 or newer.</description>
              <conditionFlags><flag name="build">new</flag></conditionFlags>
              <typeDescriptor><dependencyType><defaultType name="Optional"/><patterns>
                <pattern><dependencies><gameDependency version="{NEW_VER}"/></dependencies><type name="Recommended"/></pattern>
              </patterns></dependencyType></typeDescriptor>
            </plugin>
            <plugin name="Skyrim VR, or 1.6.1130 and older">
              <description>The older build, for Skyrim VR and for SE/AE 1.6.1130 and older. Pre-selected when the installer sees an older game version or Skyrim VR. On 1.6.1170 either build works.</description>
              <conditionFlags><flag name="build">older</flag></conditionFlags>
              <typeDescriptor><dependencyType><defaultType name="Recommended"/><patterns>
                <pattern><dependencies><gameDependency version="{NEW_VER}"/></dependencies><type name="Optional"/></pattern>
              </patterns></dependencyType></typeDescriptor>
            </plugin>
          </plugins>
        </group>
      </optionalFileGroups>
    </installStep>
  </installSteps>
  <conditionalFileInstalls>
    <patterns>
      <pattern>
        <dependencies><flagDependency flag="build" value="new"/></dependencies>
        <files><folder source="new" destination=""/></files>
      </pattern>
      <pattern>
        <dependencies><flagDependency flag="build" value="older"/></dependencies>
        <files><folder source="older" destination=""/></files>
      </pattern>
    </patterns>
  </conditionalFileInstalls>
</config>
"""
info = (f"<fomod>\n  <Name>Gourmet - AutoPatch</Name>\n  <Author>CageTV</Author>\n  <Version>{VERSION}</Version>\n"
        "  <Website>https://github.com/CageTV/Gourmet---AutoPatch</Website>\n"
        "  <Description>Makes the foods of other mods follow Gourmet - A Cooking Overhaul, with no patch plugin. "
        "Requires Gourmet - A Cooking Overhaul 1.2.0, SKSE64 and the Address Library for your game version. "
        "Two builds in one installer: pick the one for your game version.</Description>\n</fomod>\n")
xml.dom.minidom.parseString(config)
xml.dom.minidom.parseString(info)
open(os.path.join(DEST, "fomod", "ModuleConfig.xml"), "w", encoding="utf-8", newline="\n").write(config)
open(os.path.join(DEST, "fomod", "info.xml"), "w", encoding="utf-8", newline="\n").write(info)
shutil.copyfile(os.path.join(HERE, "docs", "README.txt"), os.path.join(DEST, "README.txt"))
shutil.copyfile(os.path.join(HERE, "docs", "LICENSE.txt"), os.path.join(DEST, "LICENSE.txt"))

zpath = DEST + f" {VERSION}.zip"
if os.path.exists(zpath):
    os.remove(zpath)
with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
    for dp, _, fs in os.walk(DEST):
        for fn in fs:
            p = os.path.join(dp, fn)
            z.write(p, os.path.relpath(p, DEST))
    names = z.namelist()
print(f"{zpath}: {len(names)} entries, {os.path.getsize(zpath) // 1024} KB")
for n in sorted(names):
    print("  ", n)
