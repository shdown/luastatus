# Copyright 1999-2017 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

EAPI=6
CMAKE_IN_SOURCE_BUILD=1
inherit cmake-utils

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
"

PLUGINS="
    +${PN}_plugins_alsa
    +${PN}_plugins_battery-linux
    +${PN}_plugins_cpu-usage-linux
    +${PN}_plugins_dbus
    +${PN}_plugins_file-contents-linux
    +${PN}_plugins_fs
    +${PN}_plugins_inotify
    +${PN}_plugins_imap
    +${PN}_plugins_mem-usage-linux
    +${PN}_plugins_mpd
    +${PN}_plugins_pipe
    +${PN}_plugins_timer
    +${PN}_plugins_xkb
    +${PN}_plugins_xtitle
"

LICENSE="LGPL-3+"
SLOT="0"
IUSE="doc examples luajit ${BARLIBS} ${PLUGINS}"

DEPEND=""
RDEPEND="${DEPEND}
    luajit? ( dev-lang/luajit:2 )
    !luajit? ( dev-lang/lua:0 )
    ${PN}_plugins_xtitle? ( x11-libs/libxcb x11-libs/xcb-util-wm )
    ${PN}_plugins_xkb? ( x11-libs/libX11 )
    ${PN}_plugins_alsa? ( media-libs/alsa-lib )
    ${PN}_plugins_dbus? ( dev-libs/glib )
    ${PN}_barlibs_dwm? ( x11-libs/libxcb )
    ${PN}_barlibs_i3? ( >=dev-libs/yajl-2.1.0 )
"

src_configure() {
    local mycmakeargs=(
        $(use luajit && echo -DWITH_LUA_LIBRARY=luajit)
        -DBUILD_BARLIB_DWM=$(usex ${PN}_barlibs_dwm)
        -DBUILD_BARLIB_I3=$(usex ${PN}_barlibs_i3)
        -DBUILD_BARLIB_LEMONBAR=$(usex ${PN}_barlibs_lemonbar)
        -DBUILD_PLUGIN_ALSA=$(usex ${PN}_plugins_alsa)
        -DBUILD_PLUGIN_BATTERY_LINUX=$(usex ${PN}_plugins_battery-linux)
        -DBUILD_PLUGIN_CPU_USAGE_LINUX=$(usex ${PN}_plugins_cpu-usage-linux)
        -DBUILD_PLUGIN_DBUS=$(usex ${PN}_plugins_dbus)
        -DBUILD_PLUGIN_FILE_CONTENTS_LINUX=$(usex ${PN}_plugins_file-contents-linux)
        -DBUILD_PLUGIN_FS=$(usex ${PN}_plugins_fs)
        -DBUILD_PLUGIN_INOTIFY=$(usex ${PN}_plugins_inotify)
        -DBUILD_PLUGIN_IMAP=$(usex ${PN}_plugins_imap)
        -DBUILD_PLUGIN_MEM_USAGE_LINUX=$(usex ${PN}_plugins_mem-usage-linux)
        -DBUILD_PLUGIN_MPD=$(usex ${PN}_plugins_mpd)
        -DBUILD_PLUGIN_PIPE=$(usex ${PN}_plugins_pipe)
        -DBUILD_PLUGIN_TIMER=$(usex ${PN}_plugins_timer)
        -DBUILD_PLUGIN_XKB=$(usex ${PN}_plugins_xkb)
        -DBUILD_PLUGIN_XTITLE=$(usex ${PN}_plugins_xtitle)
    )
    cmake-utils_src_configure
}

src_install() {
    default
    local i barlib plugin
    if use examples; then
        dodir /usr/share/doc/${PF}/widget-examples
        docinto widget-examples
        for i in ${BARLIBS//+/}; do
            if use ${i}; then
                barlib=${i#${PN}_barlibs_}
                dodoc -r contrib/widget-examples/${barlib}
                docompress -x /usr/share/doc/${PF}/widget-examples/${barlib}
            fi
        done
    fi

    if use doc; then
        for i in ${PLUGINS//+/}; do
            if use ${i}; then
                plugin=${i#${PN}_plugins_}
                dodir /usr/share/doc/${PF}/plugins/${plugin}
                docinto plugins/${plugin}
                dodoc plugins/${plugin}/README.md
            fi
        done

        for i in ${BARLIBS//+/}; do
            if use ${i}; then
                barlib=${i#${PN}_barlibs_}
                dodir /usr/share/doc/${PF}/barlibs/${barlib}
                docinto barlibs/${barlib}
                dodoc barlibs/${barlib}/README.md
            fi
        done
    fi
}
