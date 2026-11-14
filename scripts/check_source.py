"""Small explicit release/source checks; complements compiler warnings and review."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parent.parent
version = (root / "VERSION").read_text().strip()
assert re.fullmatch(r"\d+\.\d+\.\d+-rc\.\d+", version), "Only test-candidate versions enabled"
resource = (root / "resources/app.rc").read_text()
for key in ("FileVersion", "ProductVersion"):
    assert f'VALUE "{key}", "{version}"' in resource, f"{key} must match VERSION"
numeric = version.split("-")[0].replace(".", ",") + ",0"
assert f"FILEVERSION {numeric}" in resource
assert f"PRODUCTVERSION {numeric}" in resource
manifest = ET.parse(root / "resources/app.manifest")
ns = {"a": "urn:schemas-microsoft-com:asm.v1", "t": "urn:schemas-microsoft-com:asm.v3"}
identity = manifest.find("a:assemblyIdentity", ns)
assert identity is not None and identity.attrib["name"] == "Denberg28.Calculator"
assert identity.attrib["version"] == version.split("-")[0] + ".0"
assert identity.attrib["processorArchitecture"] == "amd64"
privilege = manifest.find("t:trustInfo/t:security/t:requestedPrivileges/t:requestedExecutionLevel", ns)
assert privilege is not None and privilege.attrib["level"] == "asInvoker"
for path in list((root / "src").glob("*")) + list((root / ".github/workflows").glob("*")):
    content = path.read_text()
    assert not re.search(r"(?:gh[pousr]_[A-Za-z0-9]{20,}|AKIA[A-Z0-9]{16}|-----BEGIN .*PRIVATE KEY-----)", content), f"Possible secret in {path}"
    if path.suffix == ".cpp":
        assert not re.search(r"\b(?:system|popen|ShellExecute[AW]?|WinExec|CreateProcess[AW]?)\s*\(", content), f"External execution in {path}"
required = ["AGENTS.md", "README.md", "CHANGELOG.md", "LICENSE", "docs/SPECIFICATION.md", "docs/ARCHITECTURE.md", "docs/TESTING.md", "docs/KNOWN_ISSUES.md"]
assert all((root / path).is_file() for path in required), "Missing project records"
print(f"PASS: source/manifest/version/privilege checks for {version}; focused secret and execution scan")
