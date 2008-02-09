#
#	rules.mk			(C) 2007-2008, Aurélien Croc (AP²C)
#
#  Compilation rules file for SpliX
#

$(rastertoqpdl_TARGET): $(rastertoqpdl_OBJ)
	$(call printCmd, $(cmd_link))
	$(Q)g++ -o $@ $^ $(rastertoqpdl_CXXFLAGS) $(rastertoqpdl_LDFLAGS) \
		$(rastertoqpdl_LIBS)

$(pstoqpdl_TARGET): $(pstoqpdl_OBJ)
	$(call printCmd, $(cmd_link))
	$(Q)g++ -o $@ $^ $(pstoqpdl_CXXFLAGS) $(pstoqpdl_LDFLAGS) \
		$(pstoqpdl_LIBS)

.PHONY: tags
tags:
	ctags --recurse --language-force=c++ --extra=+q --fields=+i \
	      --exclude=doc --exclude=.svn * 
