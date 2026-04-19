# Copyright 2026 shdown
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
PLUGINS="
  ${PN}_plugins_alsa
 +${PN}_plugins_backlight_linux
 +${PN}_plugins_battery_linux
 +${PN}_plugins_cpu_freq_linux
 +${PN}_plugins_cpu_usage_linux
 +${PN}_plugins_dbus
 +${PN}_plugins_disk_io_linux
 +${PN}_plugins_file_contents_linux
 +${PN}_plugins_fs
  ${PN}_plugins_imap
 +${PN}_plugins_inotify
 +${PN}_plugins_is_program_running
 +${PN}_plugins_mem_usage_linux
 +${PN}_plugins_mpd
 +${PN}_plugins_mpris
 +${PN}_plugins_multiplex
 +${PN}_plugins_network_linux
 +${PN}_plugins_network_rate_linux
 +${PN}_plugins_pipev2
  ${PN}_plugins_pulse
  ${PN}_plugins_systemd_unit
 +${PN}_plugins_temperature_linux
 +${PN}_plugins_timer
 +${PN}_plugins_udev
 +${PN}_plugins_unixsock
  ${PN}_plugins_web
  ${PN}_plugins_xkb
  ${PN}_plugins_xtitle
"
REQUIRED_USE="
 ${PN}_plugins_backlight_linux? ( ${PN}_plugins_udev )
 ${PN}_plugins_battery_linux? ( ${PN}_plugins_udev )
 ${PN}_plugins_cpu_freq_linux? ( ${PN}_plugins_timer )
 ${PN}_plugins_cpu_usage_linux? ( ${PN}_plugins_timer )
 ${PN}_plugins_disk_io_linux? ( ${PN}_plugins_timer )
 ${PN}_plugins_file_contents_linux? ( ${PN}_plugins_inotify )
 ${PN}_plugins_imap? ( ${PN}_plugins_timer )
 ${PN}_plugins_is_program_running? ( ${PN}_plugins_timer )
 ${PN}_plugins_mem_usage_linux? ( ${PN}_plugins_timer )
 ${PN}_plugins_mpris? ( ${PN}_plugins_dbus )
 ${PN}_plugins_network_rate_linux? ( ${PN}_plugins_timer )
 ${PN}_plugins_systemd_unit? ( ${PN}_plugins_dbus )
 ${PN}_plugins_temperature_linux? ( ${PN}_plugins_timer )
 ^^ ( lua_targets_lua5-1 lua_targets_lua5-3 lua_targets_lua5-4 lua_targets_luajit )
"

LICENSE="LGPL-3+"
SLOT="0"
IUSE="doc examples lua_targets_lua5-1 lua_targets_lua5-3 lua_targets_lua5-4 lua_targets_luajit ${BARLIBS} ${PLUGINS}"

DEPEND="
 doc? ( dev-python/docutils )
"
RDEPEND="
   lua_targets_lua5-1? ( dev-lang/lua:5.1 )
   lua_targets_lua5-3? ( dev-lang/lua:5.3 )
   lua_targets_lua5-4? ( dev-lang/lua:5.4 )
   lua_targets_luajit? ( dev-lang/luajit )
 ${PN}_barlibs_dwm? ( x11-libs/libxcb )
 ${PN}_barlibs_i3? ( dev-libs/yajl )
 ${PN}_plugins_alsa? ( media-libs/alsa-lib )
 ${PN}_plugins_dbus? ( dev-libs/glib )
 ${PN}_plugins_imap? (
   lua_targets_luajit? ( dev-lua/luasocket[lua_targets_luajit] )
   lua_targets_lua5-1? ( dev-lua/luasocket[lua_targets_lua5-1] )
   lua_targets_lua5-3? ( dev-lua/luasocket[lua_targets_lua5-3] )
   lua_targets_lua5-4? ( dev-lua/luasocket[lua_targets_lua5-4] )
   lua_targets_luajit? ( dev-lua/luasec[lua_targets_luajit] )
   lua_targets_lua5-1? ( dev-lua/luasec[lua_targets_lua5-1] )
   lua_targets_lua5-3? ( dev-lua/luasec[lua_targets_lua5-3] )
   lua_targets_lua5-4? ( dev-lua/luasec[lua_targets_lua5-4] ) )
 ${PN}_plugins_inotify? ( sys-kernel/linux-headers )
 ${PN}_plugins_network_linux? ( dev-libs/libnl )
 ${PN}_plugins_pulse? ( media-libs/libpulse )
 ${PN}_plugins_systemd_unit? ( sys-apps/systemd )
 ${PN}_plugins_udev? ( virtual/libudev )
 ${PN}_plugins_web? ( dev-libs/cJSON net-misc/curl )
 ${PN}_plugins_xkb? ( x11-libs/libX11 )
 ${PN}_plugins_xtitle? ( x11-libs/xcb-util x11-libs/xcb-util-wm x11-libs/libxcb )
"

src_configure() {
 local with_lualib
 if use lua_targets_luajit; then with_lualib=luajit; fi
 if use lua_targets_lua5-1; then with_lualib=lua5.1; fi
 if use lua_targets_lua5-3; then with_lualib=lua5.3; fi
 if use lua_targets_lua5-4; then with_lualib=lua5.4; fi
 local mycmakeargs=(
  -DWITH_LUA_LIBRARY=$with_lualib
  -DBUILD_DOCS=$(usex doc)
  -DBUILD_BARLIB_DWM=$(usex ${PN}_barlibs_dwm)
  -DBUILD_BARLIB_I3=$(usex ${PN}_barlibs_i3)
  -DBUILD_BARLIB_LEMONBAR=$(usex ${PN}_barlibs_lemonbar)
  -DBUILD_BARLIB_STDOUT=$(usex ${PN}_barlibs_stdout)
  -DBUILD_PLUGIN_ALSA=$(usex ${PN}_plugins_alsa)
  -DBUILD_PLUGIN_BACKLIGHT_LINUX=$(usex ${PN}_plugins_backlight_linux)
  -DBUILD_PLUGIN_BATTERY_LINUX=$(usex ${PN}_plugins_battery_linux)
  -DBUILD_PLUGIN_CPU_FREQ_LINUX=$(usex ${PN}_plugins_cpu_freq_linux)
  -DBUILD_PLUGIN_CPU_USAGE_LINUX=$(usex ${PN}_plugins_cpu_usage_linux)
  -DBUILD_PLUGIN_DBUS=$(usex ${PN}_plugins_dbus)
  -DBUILD_PLUGIN_DISK_IO_LINUX=$(usex ${PN}_plugins_disk_io_linux)
  -DBUILD_PLUGIN_FILE_CONTENTS_LINUX=$(usex ${PN}_plugins_file_contents_linux)
  -DBUILD_PLUGIN_FS=$(usex ${PN}_plugins_fs)
  -DBUILD_PLUGIN_IMAP=$(usex ${PN}_plugins_imap)
  -DBUILD_PLUGIN_INOTIFY=$(usex ${PN}_plugins_inotify)
  -DBUILD_PLUGIN_IS_PROGRAM_RUNNING=$(usex ${PN}_plugins_is_program_running)
  -DBUILD_PLUGIN_MEM_USAGE_LINUX=$(usex ${PN}_plugins_mem_usage_linux)
  -DBUILD_PLUGIN_MPD=$(usex ${PN}_plugins_mpd)
  -DBUILD_PLUGIN_MPRIS=$(usex ${PN}_plugins_mpris)
  -DBUILD_PLUGIN_MULTIPLEX=$(usex ${PN}_plugins_multiplex)
  -DBUILD_PLUGIN_NETWORK_LINUX=$(usex ${PN}_plugins_network_linux)
  -DBUILD_PLUGIN_NETWORK_RATE_LINUX=$(usex ${PN}_plugins_network_rate_linux)
  -DBUILD_PLUGIN_PIPEV2=$(usex ${PN}_plugins_pipev2)
  -DBUILD_PLUGIN_PULSE=$(usex ${PN}_plugins_pulse)
  -DBUILD_PLUGIN_SYSTEMD_UNIT=$(usex ${PN}_plugins_systemd_unit)
  -DBUILD_PLUGIN_TEMPERATURE_LINUX=$(usex ${PN}_plugins_temperature_linux)
  -DBUILD_PLUGIN_TIMER=$(usex ${PN}_plugins_timer)
  -DBUILD_PLUGIN_UDEV=$(usex ${PN}_plugins_udev)
  -DBUILD_PLUGIN_UNIXSOCK=$(usex ${PN}_plugins_unixsock)
  -DBUILD_PLUGIN_WEB=$(usex ${PN}_plugins_web)
  -DBUILD_PLUGIN_XKB=$(usex ${PN}_plugins_xkb)
  -DBUILD_PLUGIN_XTITLE=$(usex ${PN}_plugins_xtitle)
 )
    cmake_src_configure
}

src_install() {
    cmake_src_install
    local i
    local barlib
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
