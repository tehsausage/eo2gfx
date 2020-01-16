
macro(autogen_pch TargetName OutputFile SourceFiles)
	set (_PCHAutoGenSourceFiles "")

	foreach(File ${SourceFiles})
		list(APPEND _PCHAutoGenSourceFiles "${CMAKE_SOURCE_DIR}/${File}")
	endforeach()


	add_custom_command(OUTPUT ${OutputFile}
		COMMAND ${CMAKE_SOURCE_DIR}/autogen-pch.sh ${_PCHAutoGenSourceFiles} > ${OutputFile}
		MAIN_DEPENDENCY autogen-pch.sh
		# This dependency is commented out to avoid constant rebuilding on any source file change
		# Keeping the PCH up to date is left up to the builder
		# DEPENDS autogen-pch.sh ${SourceFiles}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)

	add_custom_target(${TargetName}
		DEPENDS ${OutputFile}
	)
endmacro()

macro(autogen_pch_copy TargetName OutputFile SourceFile)
	add_custom_command(OUTPUT ${OutputFile}
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/${SourceFile} ${OutputFile}
		MAIN_DEPENDENCY ${SourceFile}
		DEPENDS ${SourceFile}
	)

	add_custom_target(${TargetName}
		DEPENDS ${OutputFile}
	)
endmacro()
