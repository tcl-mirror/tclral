========= ral 0.7 =========
Organization of the source code did not change in this version. It is still a
single (albeit rather large) "C" file. The "relformat" utility is also still
implemented as script. However, the build procedure is now TEA compliant.

========= ral 0.6 =========
TCLRAL is contained in a single "C" source file. My apologies that the build
mechanisms are so primative. I'll improve then in future releases. As it stands
now, you need only compile the single source file to produce a shared
library.  A utility procedure, "relformat", is included as Tcl script. The
"pkgIndex.tcl" file handles the load and source necessary to get everything
loaded.
