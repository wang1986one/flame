function(generate_rc)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.rc 
	"1						VERSIONINFO\n"
	"FILEVERSION            1,0,0,0\n"
	"PRODUCTVERSION         1,0,0,0\n"
	"BEGIN\n"
	"	BLOCK \"StringFileInfo\"\n"
	"	BEGIN\n"
	"		BLOCK \"040904b0\"\n"
	"		BEGIN\n"
	"			VALUE \"FileDescription\",	\"${CMAKE_CURRENT_SOURCE_DIR}\"\n"
	"		END\n"
	"	END\n"
	"	BLOCK \"VarFileInfo\"\n"
	"	BEGIN\n"
	"		VALUE \"Translation\", 0x0409, 0x04B0\n"
	"	END\n"
	"\n"
	"END\n")
endfunction()