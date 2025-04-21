#ifndef OBJXT_PROGINFO_H
#define OBJXT_PROGINFO_H

#define OBJXT_VERSION "0.2"
#define OBJXT_PROGINFO "OBJXT - Wavefront .OBJ data extractor by Volodymyr \"Pravetz\" Didur, ver " OBJXT_VERSION
#define OBJXT_HELP(runfile) ("\nUsage:\n" + std::string(argv[0]) + " <path to .obj> <flags>\n" + \
"Possible flags:\n" + \
"\t-f = file path specifier\n" + \
"\t-of = output file name\n" + \
"\t-c = output C/C++ arrays of vertices\n" + \
"\t-py = output Python array of vertices\n" + \
"\t-v = dump vertices\n" + \
"\t-n = dump normals\n" + \
"\t-vt = dump vertex texture coordinates\n" + \
"\t-cl = set color for selected object")

#endif