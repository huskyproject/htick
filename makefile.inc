# $Id$
# include file: common part for all makefiles for htick


OBJS	=  add_desc$(OBJ) areafix$(OBJ) clean$(OBJ) fcommon$(OBJ) filecase$(OBJ)
OBJS	+= filelist$(OBJ) global$(OBJ) hatch$(OBJ) htick$(OBJ) report$(OBJ)
OBJS	+= scan$(OBJ) seenby$(OBJ) toss$(OBJ)

commonprogs: htick$(EXE)

commonclean: $(SYMBOLIC)
	-$(RM) $(RMOPT) *$(OBJ)
	-$(RM) $(RMOPT) core

commondistclean: $(SYMBOLIC)
	-$(RM) $(RMOPT) htick$(EXE)
	-$(RM) $(RMOPT) *.1.gz

htick$(EXE): $(OBJS) $(PERLOBJ)
	$(CC) $(LFLAGS) $(NAMEOPT)$@ $^ $(LIBS) $(PERLLIBS)
