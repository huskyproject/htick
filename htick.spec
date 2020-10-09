%define ver_major 1
%define ver_minor 9
%define reldate 20201009
%define reltype C
# may be one of: C (current), R (release), S (stable)

# release number for Release: header
%define relnum 3

# on default static application binary is built but using
# 'rpmbuild --without static' produces an application binary that uses
# dynamic libraries from other subprojects of Husky project
%bcond_without static

# if you use 'rpmbuild --with debug' then debug binary is produced
%bcond_with debug

# for generic build; will override for some distributions
%define vendor_prefix %nil
%define vendor_suffix %nil
%define pkg_group Applications/FTN

# for CentOS, Fedora and RHEL
%if %_vendor == "redhat"
%define vendor_suffix %dist
%endif

# for ALT Linux
%if %_vendor == "alt"
%define vendor_prefix %_vendor
%define pkg_group Networking/FTN
%endif

%define main_name htick
%if %{with static}
Name: %main_name-static
%else
Name: %main_name
%endif
Version: %ver_major.%ver_minor.%reldate%reltype
Release: %{vendor_prefix}%relnum%{vendor_suffix}
%if %_vendor != "redhat"
Group: %pkg_group
%endif
Summary: HTick - the Husky Project fileecho ticker
URL: https://github.com/huskyproject/%main_name/archive/v%ver_major.%ver_minor.%reldate.tar.gz
License: GPL
%if %{with static}
BuildRequires: huskylib-static >= 1.9, huskylib-devel >= 1.9
BuildRequires: smapi-static >= 2.5, smapi-devel >= 1.9
BuildRequires: fidoconf-static >= 1.9, fidoconf-devel >= 1.9
BuildRequires: areafix-static >= 1.9, areafix-devel >= 1.9
%else
BuildRequires: huskylib >= 1.9, huskylib-devel >= 1.9
BuildRequires: smapi >= 2.5, smapi-devel >= 1.9
BuildRequires: fidoconf >= 1.9, fidoconf-devel >= 1.9
BuildRequires: areafix >= 1.9, areafix-devel >= 1.9
Requires: huskylib >= 1.9, smapi >= 2.5, fidoconf >= 1.9, areafix >= 1.9
%endif
Source: %main_name-%ver_major.%ver_minor.%reldate.tar.gz

%description
HTick is an FTN fileecho ticker from the Husky Project

%prep
%setup -q -n %main_name-%ver_major.%ver_minor.%reldate

%build
# parallel build appears to be broken in CentOS, Fedora and RHEL
%if %_vendor == "redhat"
    %if %{with static}
        %if %{with debug}
            make DEBUG=1
        %endif
    %else
        %if %{with debug}
            make DYNLIBS=1 DEBUG=1
        %else
            make DYNLIBS=1
        %endif
    %endif
%else
    %if %{with static}
        %if %{with debug}
            %make DEBUG=1
        %endif
    %else
        %if %{with debug}
            %make DYNLIBS=1 DEBUG=1
        %else
            %make DYNLIBS=1
        %endif
    %endif
%endif
echo Install-name1:%_rpmdir/%_arch/%name-%version-%release.%_arch.rpm > /dev/null

# macro 'install' is omitted for debug build because it strips the library
%if ! %{with debug}
    %install
%endif
umask 022
%if %{with static}
    make DESTDIR=%buildroot install
%else
    make DESTDIR=%buildroot DYNLIBS=1 install
%endif
chmod -R a+rX,u+w,go-w %buildroot

%if %_vendor != "redhat"
%clean
rm -rf %buildroot
%endif

%files
%defattr(-,root,root)
%_bindir/*
%_mandir/man1/*
