# -*- RPM-SPEC -*-
# Note that this is NOT a relocatable package
%define name    redland
%define version 1.0.16
%define release SNAP
%define prefix  /usr

Summary:   Redland RDF Application Framework
Name:      %{name}
Version:   %{version}
Release:   %{release}
Prefix:    %{_prefix}
License:   LGPLv2+ or ASL 2.0
Group:     Development/Libraries
Source:    http://download.librdf.org/source/%{name}-%{version}.tar.gz
URL:       http://librdf.org/
BuildRoot: /tmp/%{name}-%{version}
BuildRequires: libxml2-devel >= 2.4.0
BuildRequires: curl-devel
BuildRequires: raptor2-devel >= 2.0.7
BuildRequires: rasqal-devel >= 0.9.25, rasqal-devel <= 0.9.99
BuildRequires: perl >= 5.8.0
BuildRequires: db4-devel
BuildRequires: mysql-devel
BuildRequires: sqlite-devel
BuildRequires: postgresql-devel
BuildRequires: gtk-doc
BuildRequires:  libtool
Packager:  Dave Beckett <dave@dajobe.org>
Docdir: %{prefix}/doc
Requires:  libxml2
Requires:  curl
Requires:  raptor2
Requires:  rasqal
Requires:  mysql

%description

Redland is a library that provides a high-level interface for RDF
(Resource Description Framework) implemented in an object-based API.
It is modular and supports different RDF parsers, serializers,
storage and query languages.  Redland is designed for developers to
provide RDF support in their applications as well as a core library
for RDF developers to start with.

%package devel
Summary: Libraries and header files for programs that use Redland.
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: raptor2 >= 2.0.7
Requires: rasqal >= 0.9.25, rasqal <= 0.9.99

%description devel
Header files for development with Redland

%prep
%setup -q

%build

%configure \
  --enable-release \
  --with-threestore=no

%{__make} OPTIMIZE="$RPM_OPT_FLAGS"

%check
make check

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%makeinstall

find $RPM_BUILD_ROOT -print | xargs chmod u+w


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_libdir}/librdf*.so.*
%{_bindir}/rdfproc
%{_bindir}/redland-db-upgrade
%dir %{_datadir}/redland
%{_datadir}/redland/mysql-v1.ttl
%{_datadir}/redland/mysql-v2.ttl

%doc AUTHORS COPYING COPYING.LIB ChangeLog LICENSE.txt NEWS README
%doc LICENSE-2.0.txt NOTICE
%doc *.html

%doc %{_mandir}/man1/redland-db-upgrade.1*
%doc %{_mandir}/man1/rdfproc.1*
%doc %{_mandir}/man3/redland.3*

%files devel
%defattr(-, root, root,-)
%{_bindir}/redland-config
%{_libdir}/librdf*.a
%{_libdir}/librdf*.la
%{_libdir}/librdf*.so
%{_libdir}/pkgconfig/redland.pc
%dir %{_datadir}/redland
%{_datadir}/redland/Redland.i

%{_includedir}/redland.h
%{_includedir}/librdf.h
%{_includedir}/rdf_*.h

%doc AUTHORS COPYING COPYING.LIB ChangeLog LICENSE.txt NEWS README
%doc LICENSE-2.0.txt NOTICE

%doc docs/README.html
%doc %{_datadir}/gtk-doc/html

%doc %{_mandir}/man1/redland-config.1*

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%changelog
* Tue Feb 1 2011  Dave Beckett <dave@dajobe.org>
- Require Raptor 2
- Do not use removed configure options --with-raptor and --with-rasqal

* Sun Dec 16 2007  Dave Beckett <dave@dajobe.org>
- rasqal-devel has a max and a min version now
- use macros like %{_bindir} and %{_includedir}
- add postgresql-devel, gtk-doc and libtool to BuildRequires
- create some new dirs explicitly

* Sat May  5 2007  Dave Beckett <dave@dajobe.org>
- Add /usr/share/redland/mysql-v1.ttl and /usr/share/redland/mysql-v2.ttl

* Wed Feb 15 2006  Dave Beckett <dave@dajobe.org>
- Require db4-devel
- Disable postgresql for now

* Thu Aug 11 2005  Dave Beckett <dave.beckett@bristol.ac.uk>
- Update Source:
- Do not require python-devel at build time
- Add sqlite-devel build requirement.
- Use %configure and %makeinstall

* Thu Jul 21 2005  Dave Beckett <dave.beckett@bristol.ac.uk>
- Updated for gtk-doc locations

* Mon Nov 1 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- License now LGPL/Apache 2
- Added LICENSE-2.0.txt and NOTICE

* Mon Jul 19 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- move perl, python packages into redland-bindings

* Mon Jul 12 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- put /usr/share/redland/Redland.i in redland-devel

* Wed May  5 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- require raptor 1.3.0
- require rasqal 0.2.0

* Fri Jan 30 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- require raptor 1.2.0
- update for removal of python distutils
- require python 2.2.0+
- require perl 5.8.0+
- build and require mysql
- do not build and require threestore

* Sun Jan 4 2004  Dave Beckett <dave.beckett@bristol.ac.uk>
- added redland-python package
- export some more docs

* Mon Dec 15 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- require raptor 1.1.0
- require libxml 2.4.0 or newer
- added pkgconfig redland.pc
- split redland/devel package shared libs correctly

* Mon Sep 8 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- require raptor 1.0.0
 
* Thu Sep 4 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- added rdfproc
 
* Thu Aug 28 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- patches added post 0.9.13 to fix broken perl UNIVERSAL::isa
 
* Thu Aug 21 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- Add redland-db-upgrade.1
- Removed duplicate perl CORE shared objects

* Sun Aug 17 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- Updates for new perl module names.

* Tue Apr 22 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- Updated for Redhat 9, RPM 4

* Fri Feb 12 2003 Dave Beckett <dave.beckett@bristol.ac.uk>
- Updated for redland 0.9.12

* Fri Jan 4 2002 Dave Beckett <dave.beckett@bristol.ac.uk>
- Updated for new Perl module names

* Fri Sep 14 2001 Dave Beckett <dave.beckett@bristol.ac.uk>
- Added shared libraries
