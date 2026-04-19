# Generated automatically.

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

###${___paste_useflags___}

LICENSE="LGPL-3+"
SLOT="0"
IUSE="doc examples lua_targets_lua5-1 lua_targets_lua5-3 lua_targets_lua5-4 lua_targets_luajit ${BARLIBS} ${PLUGINS}"

###${___paste_depends___}

src_configure() {
###${___paste_configure___}
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
