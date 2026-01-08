# Copyright 1999-2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

# to find config.generated.h
CMAKE_IN_SOURCE_BUILD=true
LUA_COMPAT=( lua5-{1..4} luajit )
inherit cmake edo lua-single

DESCRIPTION="Universal status bar content generator"
HOMEPAGE="https://github.com/shdown/luastatus"
if [[ ${PV} == *9999* ]]; then
	inherit git-r3
	EGIT_REPO_URI="https://github.com/shdown/${PN}.git"
else
	SRC_URI="https://github.com/shdown/${PN}/archive/v${PV}.tar.gz -> ${P}.tar.gz"
	KEYWORDS="~amd64 ~x86"
fi

LICENSE="LGPL-3+"
SLOT="0"
IUSE="alsa dbus i3 imap network pulseaudio test udev X"
REQUIRED_USE="${LUA_REQUIRED_USE}"
RESTRICT="!test? ( test )"

RDEPEND="
	${LUA_DEPS}
	alsa? ( >=media-libs/alsa-lib-1.0.27.2 )
	dbus? (
		>=dev-libs/glib-2.40.2:2[dbus]
		sys-apps/dbus
	)
	i3? ( >=dev-libs/yajl-2.0.4:= )
	imap? (
		$(lua_gen_cond_dep '
			dev-lua/luasec[${LUA_USEDEP}]
			dev-lua/luasocket[${LUA_USEDEP}]
		')
	)
	network? ( dev-libs/libnl:3 )
	pulseaudio? ( >=media-libs/libpulse-4.0 )
	udev? ( >=virtual/libudev-204:= )
	X? (
		>=x11-libs/libxcb-1.10:=
		>=x11-libs/libX11-1.6.2
		>=x11-libs/xcb-util-wm-0.4.1
	)
"
# linux-headers for inotify and network
DEPEND="
	${RDEPEND}
	sys-kernel/linux-headers
	X? ( >=x11-libs/xcb-util-0.3.8 )
"
BDEPEND="
	dev-python/docutils
	virtual/pkgconfig
	test? (
		i3? ( app-misc/jq )
		pulseaudio? ( media-sound/pulseaudio-daemon )
	)
"

src_configure() {
	local mycmakeargs=(
		-DBUILD_TESTS=$(usex test)
		-DWITH_LUA_LIBRARY=${ELUA}

		-DBUILD_BARLIB_I3=$(usex i3)
		-DBUILD_BARLIB_DWM=$(usex X)

		-DLUA_PLUGINS_DIR="$(lua_get_lmod_dir)/${PN}/plugins"
		# handle only plugins with deps, others are enabled by default
		-DBUILD_PLUGIN_ALSA=$(usex alsa)
		-DBUILD_PLUGIN_DBUS=$(usex dbus)
		-DBUILD_PLUGIN_NETWORK_LINUX=$(usex network)
		-DBUILD_PLUGIN_PULSE=$(usex pulseaudio)
		-DBUILD_PLUGIN_BACKLIGHT_LINUX=$(usex udev)
		-DBUILD_PLUGIN_BATTERY_LINUX=$(usex udev)
		-DBUILD_PLUGIN_UDEV=$(usex udev)
		-DBUILD_PLUGIN_UNIXSOCK=ON
		-DBUILD_PLUGIN_XKB=$(usex X)
		-DBUILD_PLUGIN_XTITLE=$(usex X)
	)
	cmake_src_configure
}

src_test() {
	local skip_tests=(
		plugin-mpd # needs server
		plugin-llamacxx # disabled
		httpserv # for llamacxx, needs libwebsocket
		$(usev !dbus plugin-dbus)
		$(usev !i3 barlib-i3)
		$(usev !imap plugin-imap)
		$(usev !network plugin-network-linux)
		$(usev !pulseaudio plugin-pulse)
		$(usev !udev plugin-backlight-linux)
		$(usev !udev plugin-battery-linux)
	)
	edo ./tests/pt.sh "${BUILD_DIR}" "${skip_tests[@]/#/skip:}"
}

src_install() {
	cmake_src_install

	dodoc -r examples
	docompress -x /usr/share/doc/${PF}/examples
}
