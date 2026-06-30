set(PCI_IDS_HEADER_OUTPUT "${HWINFO_CMAKE_BINARY_DIR}/generated/pci.ids.h")

add_custom_command(
        OUTPUT ${PCI_IDS_HEADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${HWINFO_CMAKE_BINARY_DIR}/generated"
        COMMAND ${CMAKE_COMMAND}
        -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/pci.ids
        -DOUTPUT=${PCI_IDS_HEADER_OUTPUT}
        -DNAME=pci_ids
        -P ${CMAKE_CURRENT_SOURCE_DIR}/embed.cmake
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/pci.ids" "${CMAKE_CURRENT_SOURCE_DIR}/embed.cmake"
        COMMENT "Generating pci_ids header"
        VERBATIM
)
add_custom_target(generate_pci_ids_header
        DEPENDS
        "${PCI_IDS_HEADER_OUTPUT}"
)