bool pcbWriteDesign(dbDesign design, char *fileName);
bool pcbWriteNetlist(dbNetlist netlist, char *fileName);
dbDesign pcbReadDesign(char *designName, char *fileName, dbDesign libDesign);
