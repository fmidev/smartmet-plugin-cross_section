%define DIRNAME cross_section
%define SPECNAME smartmet-plugin-cross-section
Summary: SmartMet Cross-Section plugin
Name: %{SPECNAME}
Version: 17.4.8

Release: 1%{?dist}.fmi
License: FMI
Group: SmartMet/Plugins
URL: https://github.com/fmidev/smartmet-plugin-cross_section
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: make
BuildRequires: boost-devel
BuildRequires: libconfig >= 1.4.9
BuildRequires: smartmet-library-macgyver-devel >= 17.3.16
BuildRequires: smartmet-library-spine-devel >= 17.4.8
BuildRequires: smartmet-engine-querydata-devel >= 17.4.8
BuildRequires: smartmet-engine-geonames-devel >= 17.3.15
BuildRequires: smartmet-engine-contour-devel >= 17.3.15
BuildRequires: ctpp2 >= 2.8.2
BuildRequires: jsoncpp-devel >= 0.10.5
Requires: jsoncpp >= 0.10.5
Requires: ctpp2 >= 2.8.2
Requires: libconfig
Requires: smartmet-library-macgyver >= 17.3.16
Requires: smartmet-engine-querydata >= 17.4.8
Requires: smartmet-engine-geonames >= 17.3.15
Requires: smartmet-engine-contour >= 17.3.15
Requires: smartmet-server >= 17.4.7
Requires: smartmet-library-spine >= 17.4.8
Requires: boost-date-time
Requires: boost-iostreams
Requires: boost-system
Requires: boost-thread
Provides: %{SPECNAME}
Obsoletes: smartmet-brainstorm-csection < 16.11.1
Obsoletes: smartmet-brainstorm-csection-debuginfo < 16.11.1

%description
SmartMet Cross-Section plugin

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{SPECNAME}
 
%build -q -n %{SPECNAME}
make %{_smp_mflags}

%install
%makeinstall
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/smartmet/plugins
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/smartmet/plugins/cross_section

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,0775)
%{_datadir}/smartmet/plugins/%{DIRNAME}.so
%defattr(0664,root,root,0775)
%{_sysconfdir}/smartmet/plugins/%{DIRNAME}/*.c2t

%changelog
* Sat Apr  8 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.4.8-1.fmi
- Simplified error reporting

* Mon Apr  3 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.4.3-1.fmi
- Added CORS headers

* Wed Mar 15 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.3.15-1.fmi
- Recompiled since Spine::Exception changed

* Sat Feb 11 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.11-1.fmi
- Repackaged due to newbase API change

* Sat Feb  4 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.4-1.fmi
- Changed root directory from an optional to a required parameter

* Mon Jan 30 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.30-1.fmi
- Require latest contour engine to fix problems with self touching isolines

* Wed Jan  4 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.4-1.fmi
- Changed to use renamed SmartMet base libraries

* Tue Nov 29 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.30-1.fmi
- Refactored configuration files

* Tue Nov 29 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.29-1.fmi
- Recompiled due to API changes

* Thu Nov  3 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.1-2.fmi
- Renamed variables and paths to use underscore since libconfig disallows hyphen in variable names

* Tue Nov  1 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.1-1.fmi
- Namespace and directory name changed

* Tue Sep 13 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.9.13-1.fmi
- Code modified bacause of Contour-engine API changes: vector of isolines/isobands queried at once instead of only one isoline/isoband

* Tue Sep  6 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.9.6-1.fmi
- New exception handler

* Tue Aug 30 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.8.30-1.fmi
- Base class API change
- Use response code 400 instead of 503

* Mon Aug 15 2016 Markku Koskela <markku.koskela@fmi.fi> - 16.8.15-1.fmi
- The init(),shutdown() and requestHandler() methods are now protected methods
- The requestHandler() method is called from the callRequestHandler() method

* Wed Jun 29 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.29-1.fmi
- QEngine API changed

* Tue Jun 14 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.14-1.fmi
- Full recompile

* Thu Jun  2 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.2-1.fmi
- Full recompile

* Wed Jun  1 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.1-1.fmi
- Added graceful shutdown

* Mon May 16 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.5.16-1.fmi
- Use TimeZones instead of TimeZoneFactory

* Wed Apr 20 2016 Tuomo Lauri <tuomo.lauri@fmi.fi> - 16.4.20-1.fmi
- Built against new Qengine and Contour - engine
- Switched to use the latest TimeSeriesGenerator API which handles climatological data too

* Mon Apr 18 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.4.18-1.fmi
- Recompiled with the latest jsoncpp

* Sat Jan 23 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.23-1.fmi
- API changes in newbase and macgyver 

* Mon Jan 18 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.18-1.fmi
- newbase API changed, full recompile

* Wed Nov 18 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.11.18-1.fmi
- SmartMetPlugin now receives a const HTTP Request

* Mon Oct 26 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.10.26-1.fmi
- Added proper debuginfo packaging

* Thu Aug 27 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.27-1.fmi
- TimeSeriesGenerator API changed

* Mon Aug 24 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.24-1.fmi
- Recompiled due to Convenience.h API changes

* Tue Aug 18 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.18-1.fmi
- Recompile forced by brainstorm API changes

* Mon Aug 17 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.17-1.fmi
- Use -fno-omit-frame-pointer to improve perf use

* Fri Aug 14 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.14-1.fmi
- Recompiled due to string formatting changes

* Mon May 25 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.5.25-1.fmi
- Rebuilt against new QEngine

* Wed Apr 29 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.29-1.fmi
- Removed optionengine depencency

* Thu Apr  9 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.9-1.fmi
- newbase API changed

* Wed Apr  8 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.8-1.fmi
- Dynamic linking of smartmet libraries into use

* Tue Feb 24 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.2.24-1.fmi
- Recompiled due to changes in newbase linkage

* Thu Dec 18 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.12.18-1.fmi
- Recompiled due to spine API changes

* Mon Sep  8 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.9.8-1.fmi
- Recompiled due to geoengine API changes

* Tue Aug 26 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.8.26-1.fmi
- Added support for countouring with respect to another parameter as the level value (usually GeomHeight)

* Wed Aug  6 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.8.6-1.fmi
- Linked with latest newbase with corrected WKT methods
- Fri Aug  1 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.8.1-1.fmi
- Return paths even for empty cross sections

* Thu Jul 31 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.7.31-1.fmi
- Initial version
