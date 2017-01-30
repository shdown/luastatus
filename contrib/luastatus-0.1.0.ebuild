# Copyright 1999-2017 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Id$

EAPI=6
CMAKE_IN_SOURCE_BUILD=1
inherit cmake-utils

DESCRIPTION="Universal status bar content generator"
HOMEPAGE="https://github.com/shdown/luastatus"

if [[ ${PV} == *9999* ]]; then
	inherit git-r3
	SRC_URI=""
	EGIT_REPO_URI="git@github.com:shdown/${PN}.git"
	KEYWORDS="~amd64 ~x86"
else
	SRC_URI="https://github.com/shdown/${PN}/archive/v${PV}.tar.gz -> ${P}.tar.gz"
	KEYWORDS="amd64 x86"
fi

BARLIBS="
	${PN}_barlibs_dwm
	${PN}_barlibs_i3
	${PN}_barlibs_lemonbar
"

PLUGINS="
	+${PN}_plugins_alsa
	+${PN}_plugins_fs
	+${PN}_plugins_mpd
	+${PN}_plugins_pipe
	+${PN}_plugins_timer
	+${PN}_plugins_xkb
	+${PN}_plugins_xtitle
"

LICENSE="GPL-3"
SLOT="0"
IUSE="${BARLIBS} ${PLUGINS}"

DEPEND=""
RDEPEND="${DEPEND}
	dev-lang/lua
	dev-lang/luajit
	${PN}_plugins_xtitle? ( 
		x11-libs/libxcb 
		x11-libs/xcb-util-wm 
	)
	${PN}_plugins_xkb? ( 
		x11-libs/libX11 
		x11-libs/libxcb 
	)
	${PN}_plugins_alsa? ( media-libs/alsa-lib )
	${PN}_barlibs_dwm? ( 
		x11-wm/dwm 
		x11-libs/libxcb 
	)
	${PN}_barlibs_i3? ( 
		x11-wm/i3 
		>=dev-libs/yajl-2.1.0 
	)
	${PN}_barlibs_lemonbar? ( x11-misc/lemonbar )
"

src_configure() {
	local mycmakeargs=(
		-DBUILD_BARLIB_DWM=$( usex ${PN}_barlibs_dwm )
		-DBUILD_BARLIB_I3=$( usex ${PN}_barlibs_i3 )
		-DBUILD_BARLIB_LEMONBAR=$( usex ${PN}_barlibs_lemonbar )
		-DBUILD_PLUGIN_ALSA=$(usex ${PN}_plugins_alsa )
		-DBUILD_PLUGIN_FS=$(usex ${PN}_plugins_fs )
		-DBUILD_PLUGIN_MPD=$(usex ${PN}_plugins_mpd )
		-DBUILD_PLUGIN_PIPE=$(usex ${PN}_plugins_pipe )
		-DBUILD_PLUGIN_TIMER=$(usex ${PN}_plugins_timer )
		-DBUILD_PLUGIN_XKB=$(usex ${PN}_plugins_xkb )
		-DBUILD_PLUGIN_XTITLE=$(usex ${PN}_plugins_xtitle)
	)
	cmake-utils_src_configure
}
