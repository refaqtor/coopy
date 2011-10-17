
#include <stdio.h>
#include <coopy/Diff.h>

using namespace coopy::app;

int main(int argc, char *argv[]) {
  Options opt("sspatch");
  int r = opt.apply(argc,argv);
  if (r!=0) return r;

  bool help = opt.checkBool("help");
  if (argc==1 || help) {
    opt.beginHelp();
    opt.addUsage("sspatch [options] DATAFILE PATCHFILE");
    opt.addDescription("Modify a table/database/spreadsheet to integrate the changes described\nin a pre-computed difference.");
    opt.showOptions(OPTION_FOR_PATCH);
    opt.endHelp();
    return 1;
  }

  Diff diff;
  return diff.apply(opt);
}
