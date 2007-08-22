bool vrWriteDesign(dbDesign design, char *fileName, bool wholeLibrary);
bool vrWriteNetlist(dbNetlist netlist, char *fileName);
dbDesign vrReadDesign(char *designName, char *fileName, dbDesign libDesign);
