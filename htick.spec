%global ver_major 1
%global ver_minor 9
%global ver_patch 0
%global reldate 20201016
%global reltype C
# may be one of: C (current), R (release), S (stable)

# release number for Release: header
%global relnum 3

# on default static application binary is built but using
# 'rpmbuild --without static' produces an application binary that uses
# dynamic libraries from other subprojects of Husky project
%if %_vendor == "alt"
    %def_with static
    %global  __arch_install_post %nil
%else
    %bcond_without static
%endif

# if you use 'rpmbuild --with debug' then debug binary is produced
%if %_vendor == "alt"
    %def_without debug
%else
    %bcond_with debug
%endif

%global debug_package %nil

# for generic build; will override for some distributions
%global vendor_prefix %nil
%global vendor_suffix %nil
%global pkg_group Applications/Communications

# for CentOS, Fedora and RHEL
%if %_vendor == "redhat"
    %global vendor_suffix %dist
%endif

# for ALT Linux
%if %_vendor == "alt"
    %global vendor_prefix %_vendor
    %global pkg_group Networking/FTN
%endif

%global main_name htick
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
%if %{with static}
    %if %{with debug}
        %make_build DEBUG=1
    %else
        %make_build
    %endif
%else
    %if %{with debug}
        %make_build DYNLIBS=1 DEBUG=1
    %else
        %make_build DYNLIBS=1
    %endif
%endif
echo Install-name1:%_rpmdir/%_arch/%name-%version-%release.%_arch.rpm > /dev/null

%if %{with debug}
    %global __os_install_post %nil
%endif

%install
umask 022
%if %{with static}
    make DESTDIR=%buildroot install
%else
    make DESTDIR=%buildroot DYNLIBS=1 install
%endif
chmod -R a+rX,u+w,go-w %buildroot

%files
%defattr(-,root,root)
%_bindir/*
%_mandir/man1/*
