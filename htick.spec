%define reldate 20160318
%define reltype C
# may be one of: C (current), R (release), S (stable)

Name: htick
Version: 1.9.%{reldate}%{reltype}
Release: 1
Group: Applications/FTN
Summary: HTick - the Husky Project fileecho ticker
URL: http://husky.sf.net
License: GPL
BuildRequires: fidoconf >= 1.9, smapi >= 2.5, areafix >= 1.9
Source: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
HTick is the FTN fileecho ticker from the Husky Project.

%prep
%setup -q -n %{name}

%build
sed -i -re 's,OPTLFLAGS=-s,OPTLFLAGS=-s -static,g' huskymak.cfg
make

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_prefix}/*
