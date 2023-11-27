# Copyright 1999-2023 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8
CMAKE_IN_SOURCE_BUILD=1
inherit cmake

DESCRIPTION="Universal status bar content generator"
HOMEPAGE="https://github.com/shdown/luastatus"

if [[ ${PV} == *9999* ]]; then
	inherit git-r3
	SRC_URI=""
	EGIT_REPO_URI="https://github.com/shdown/${PN}.git"
	KEYWORDS="~amd64 ~x86"
else
	SRC_URI="https://github.com/shdown/${PN}/archive/v${PV}.tar.gz -> ${P}.tar.gz"
	KEYWORDS="amd64 x86"
fi

BARLIBS="
	${PN}_barlibs_dwm
	${PN}_barlibs_i3
	${PN}_barlibs_lemonbar
	${PN}_barlibs_stdout
"

PROPER_PLUGINS="
	+${PN}_plugins_alsa
	+${PN}_plugins_dbus
	+${PN}_plugins_fs
	+${PN}_plugins_inotify
	+${PN}_plugins_mpd
	+${PN}_plugins_network-linux
	+${PN}_plugins_pulse
	+${PN}_plugins_timer
	+${PN}_plugins_udev
	+${PN}_plugins_unixsock
	+${PN}_plugins_xkb
	+${PN}_plugins_xtitle
"

DERIVED_PLUGINS="
	+${PN}_plugins_backlight-linux
	+${PN}_plugins_battery-linux
	+${PN}_plugins_cpu-usage-linux
	+${PN}_plugins_file-contents-linux
	+${PN}_plugins_imap
	+${PN}_plugins_mem-usage-linux
	+${PN}_plugins_pipe
"

PLUGINS="
	${PROPER_PLUGINS}
	${DERIVED_PLUGINS}
"

LICENSE="LGPL-3+"
SLOT="0"
IUSE="doc examples luajit ${BARLIBS} ${PLUGINS}"
REQUIRED_USE="
	${PN}_plugins_backlight-linux? ( ${PN}_plugins_udev )
	${PN}_plugins_battery-linux? ( ${PN}_plugins_udev )
	${PN}_plugins_cpu-usage-linux? ( ${PN}_plugins_timer )
	${PN}_plugins_file-contents-linux? ( ${PN}_plugins_inotify )
	${PN}_plugins_imap? ( ${PN}_plugins_timer )
	${PN}_plugins_mem-usage-linux? ( ${PN}_plugins_timer )
	${PN}_plugins_pipe? ( ${PN}_plugins_timer )
"

DEPEND="
	doc? ( dev-python/docutils )
"
RDEPEND="
	luajit? ( dev-lang/luajit:2 )
	!luajit? ( dev-lang/lua )
	${PN}_barlibs_dwm? ( x11-libs/libxcb )
	${PN}_barlibs_i3? ( >=dev-libs/yajl-2.0.4 )
	${PN}_plugins_alsa? ( media-libs/alsa-lib )
	${PN}_plugins_dbus? ( dev-libs/glib )
	${PN}_plugins_network-linux? ( sys-kernel/linux-headers dev-libs/libnl )
	${PN}_plugins_pulse? ( media-sound/pulseaudio )
	${PN}_plugins_udev? ( virtual/libudev )
	${PN}_plugins_xkb? ( x11-libs/libX11 )
	${PN}_plugins_xtitle? ( x11-libs/libxcb x11-libs/xcb-util-wm x11-libs/xcb-util )
"

src_configure() {
	local mycmakeargs=(
		$(use luajit && echo -DWITH_LUA_LIBRARY=luajit)
		-DBUILD_DOCS=$(usex doc)
		-DBUILD_BARLIB_DWM=$(usex ${PN}_barlibs_dwm)
		-DBUILD_BARLIB_I3=$(usex ${PN}_barlibs_i3)
		-DBUILD_BARLIB_LEMONBAR=$(usex ${PN}_barlibs_lemonbar)
		-DBUILD_BARLIB_STDOUT=$(usex ${PN}_barlibs_stdout)
		-DBUILD_PLUGIN_ALSA=$(usex ${PN}_plugins_alsa)
		-DBUILD_PLUGIN_BACKLIGHT_LINUX=$(usex ${PN}_plugins_backlight-linux)
		-DBUILD_PLUGIN_BATTERY_LINUX=$(usex ${PN}_plugins_battery-linux)
		-DBUILD_PLUGIN_CPU_USAGE_LINUX=$(usex ${PN}_plugins_cpu-usage-linux)
		-DBUILD_PLUGIN_DBUS=$(usex ${PN}_plugins_dbus)
		-DBUILD_PLUGIN_FILE_CONTENTS_LINUX=$(usex ${PN}_plugins_file-contents-linux)
		-DBUILD_PLUGIN_FS=$(usex ${PN}_plugins_fs)
		-DBUILD_PLUGIN_IMAP=$(usex ${PN}_plugins_imap)
		-DBUILD_PLUGIN_INOTIFY=$(usex ${PN}_plugins_inotify)
		-DBUILD_PLUGIN_MEM_USAGE_LINUX=$(usex ${PN}_plugins_mem-usage-linux)
		-DBUILD_PLUGIN_MPD=$(usex ${PN}_plugins_mpd)
		-DBUILD_PLUGIN_NETWORK_LINUX=$(usex ${PN}_plugins_network-linux)
		-DBUILD_PLUGIN_PIPE=$(usex ${PN}_plugins_pipe)
		-DBUILD_PLUGIN_PULSE=$(usex ${PN}_plugins_pulse)
		-DBUILD_PLUGIN_TIMER=$(usex ${PN}_plugins_timer)
		-DBUILD_PLUGIN_UDEV=$(usex ${PN}_plugins_udev)
		-DBUILD_PLUGIN_UNIXSOCK=$(usex ${PN}_plugins_unixsock)
		-DBUILD_PLUGIN_XKB=$(usex ${PN}_plugins_xkb)
		-DBUILD_PLUGIN_XTITLE=$(usex ${PN}_plugins_xtitle)
	)
	cmake_src_configure
}

src_install() {
	cmake_src_install
	local i
	if use examples; then
		dodir /usr/share/doc/${PF}/examples
		docinto examples
		for i in ${BARLIBS//+/}; do
			if use ${i}; then
				barlib=${i#${PN}_barlibs_}
				dodoc -r examples/${barlib}
				docompress -x /usr/share/doc/${PF}/examples/${barlib}
			fi
		done
	fi
}
