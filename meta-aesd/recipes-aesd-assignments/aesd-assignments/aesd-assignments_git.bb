# See https://git.yoctoproject.org/poky/tree/meta/files/common-licenses
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

#inherit update-rc.d to install my app in the start system

# TODO: Set this  with the path to your assignments rep.  Use ssh protocol and see lecture notes
# about how to setup ssh-agent for passwordless access
SRC_URI = "git://git@github.com/cu-ecen-aeld/assignments-3-and-later-clavij0;protocol=ssh;branch=main"
 
PV = "1.0+git${SRCPV}"
# TODO: set to reference a specific commit hash in your assignment repo
SRCREV = "2eb5e04ba529973a479b4ed5dbb0c29eabe98421"

# This sets your staging directory based on WORKDIR, where WORKDIR is defined at 
# https://docs.yoctoproject.org/ref-manual/variables.html?highlight=workdir#term-WORKDIR
# We reference the "server" directory here to build from the "server" directory
# in your assignments repo
S = "${WORKDIR}/git/server"

# TODO: Add the aesdsocket application and any other files you need to install
# See https://git.yoctoproject.org/poky/plain/meta/conf/bitbake.conf?h=kirkstone
FILES:${PN} += "${bindir}/aesdsocket"
#FILES:${PN} += "${S}/startup-aesdsocket.sh"


# TODO: customize these as necessary for any libraries you need for your application
# (and remove comment)
TARGET_LDFLAGS += "-pthread -lrt"

#install Packages
inherit update-rc.d 
INITSCRIPT_PACKAGES="${PN}"
INITSCRIPT_NAME:${PN} = "startup-aesdsocket.sh"
#INITSCRIPT_PARAMS = "defaults"


do_configure () {
	:
}

do_compile () {
	oe_runmake
}

do_install () {
	# TODO: Install your binaries/scripts here.
	# Be sure to install the target directory with install -d first
	# Yocto variables ${D} and ${S} are useful here, which you can read about at 
	# https://docs.yoctoproject.org/ref-manual/variables.html?highlight=workdir#term-D
	# and
	# https://docs.yoctoproject.org/ref-manual/variables.html?highlight=workdir#term-S
	# See example at https://github.com/cu-ecen-aeld/ecen5013-yocto/blob/ecen5013-hello-world/meta-ecen5013/recipes-ecen5013/ecen5013-hello-world/ecen5013-hello-world_git.bb
	#bindir -> /usr/bin https://wiki.koansoftware.com/index.php/Directories_and_installation_variables
	install -d ${D}${bindir}
	install -m 0755 ${S}/${INITSCRIPT_PACKAGES} ${D}${bindir}/
	install -m 0755 ${S}/${INITSCRIPT_NAME} ${D}${bindir}/

	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${S}/${INITSCRIPT_PACKAGES} ${D}${sysconfdir}/init.d
	install -m 0755 ${S}/${INITSCRIPT_NAME} ${D}${sysconfdir}/init.d


}
