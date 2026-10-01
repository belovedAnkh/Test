vcpkg_from_git(
	OUT_SOURCE_PATH SOURCE_PATH
	URL https://github.com/CharmedBaryon/CommonLibSSE-NG
	REF b93280e832f263dbef44e44cbe2936622a02f91a
)

vcpkg_cmake_configure(
	SOURCE_PATH "${SOURCE_PATH}"
	OPTIONS
		-DBUILD_TESTS=OFF
		-DENABLE_SKYRIM_SE=ON
		-DENABLE_SKYRIM_AE=ON
		-DENABLE_SKYRIM_VR=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME CommonLibSSE CONFIG_PATH lib/cmake/CommonLibSSE)

# The upstream install rules do not ship the add_commonlibsse_plugin() helper that CommonLibSSEConfig.cmake includes.
file(INSTALL "${SOURCE_PATH}/cmake/CommonLibSSE.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
