
Summary: Tools to manage the Linux NetLabel subsystem
Name: netlabel_tools
Version: X.XX
Release: 1
License: GPL
Group: System Environment/Daemons
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
NetLabel is a kernel subsystem which implements explicit packet labeling
protocols such as CIPSO and RIPSO for Linux.  Packet labeling is used in
secure networks to mark packets with the security attributes of the data they
contain.  This package provides the necessary user space tools to query and
configure the kernel subsystem.

%prep 
%setup -n %{name}-%{version}

%build
make

%install
rm -rf $RPM_BUILD_ROOT
make INSTALL_PREFIX=${RPM_BUILD_ROOT} \
     INSTALL_MAN_DIR=${RPM_BUILD_ROOT}/usr/share/man \
     OWNER=`id -nu` GROUP=`id -ng` install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc docs/*.txt
%attr(0755,root,root) /sbin/*
%attr(0644,root,root) %{_mandir}/man8/*

%changelog
* Thu Sep 28 2006 Paul Moore <paul.moore@hp.com> X.XX-1
- This specfile will no longer be updated for individual release numbers,
  however, it will be kept up to date so that it could be used to build
  a netlabel_tools RPM once the correct version information has been entered

* Thu Aug  3 2006 Paul Moore <paul.moore@hp.com> 0.16-1
- Bumped version number.

* Thu Jul  6 2006 Paul Moore <paul.moore@hp.com> 0.15-1
- Bumped version number.

* Mon Jun 26 2006 Paul Moore <paul.moore@hp.com> 0.14-1
- Bumped version number.
- Changes related to including the version number in the path name.
- Changed the netlabelctl perms from 0750 to 0755.
- Removed the patch. (included in the base with edits)
- Updated the description.

* Fri Jun 23 2006 Steve Grubb <sgrubb@redhat.com> 0.13-1
- Initial build.

