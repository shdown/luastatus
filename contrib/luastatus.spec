Name:           luastatus
Version:        0.1.0
Release:        1%{?dist}
Summary:        Lua status

License:        LGPL3+
URL:            https://github.com/shdown/luastatus
Source0:        https://github.com/shdown/luastatus/archive/v%version.tar.gz

BuildRequires:  cmake
BuildRequires:  luajit-devel
BuildRequires:  libxcb-devel
BuildRequires:  yajl-devel
BuildRequires:  alsa-lib-devel
BuildRequires:  xcb-util-wm-devel
BuildRequires:	xcb-util-devel

%description
Lua status daemon

%package plugins
Summary:	Lua status plugins
Requires:	%{name} = %{version}-%{release}

%description plugins
Lua status plugins

%prep
%setup -q

%build
%cmake -DWITH_LUA_LIBRARY=luajit .

%make_build

%install
%make_install

%files
%doc LICENSE.txt README.md
%{_bindir}/luastatus
%{_bindir}/luastatus-i3-wrapper
%{_mandir}/man1/luastatus.1*
%{_libdir}/luastatus/barlibs/*.so

%files plugins
%{_libdir}/luastatus/plugins/*.so

%changelog

