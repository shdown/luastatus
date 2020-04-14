Name:           luastatus
Version:        0.3.0
Release:        1%{?dist}
Summary:        universal statusbar content generator

License:        LGPL3+
URL:            https://github.com/shdown/luastatus
Source0:        https://github.com/shdown/luastatus/archive/v%version.tar.gz

BuildRequires:  cmake
BuildRequires:  luajit-devel
BuildRequires:  libxcb-devel
BuildRequires:  yajl-devel
BuildRequires:  alsa-lib-devel
BuildRequires:  xcb-util-wm-devel
BuildRequires:  xcb-util-devel
BuildRequires:  glib2-devel

%description
a universal status bar content generator

%package plugins
Summary:    luastatus plugins
Requires:   %{name} = %{version}-%{release}

%description plugins
luastatus plugins

%prep
%setup -q

%build
%cmake -DWITH_LUA_LIBRARY=luajit .

%make_build

%install
%make_install

%files
%doc COPYING.txt COPYING.LESSER.txt README.md
%{_bindir}/luastatus
%{_bindir}/luastatus-i3-wrapper
%{_bindir}/luastatus-lemonbar-launcher
%{_mandir}/man1/luastatus.1*
%{_libdir}/luastatus/barlibs/*

%files plugins
%{_libdir}/luastatus/plugins/*

%changelog

