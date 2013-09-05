#!/bin/bash
set -e
if [ "X$VERBOSE" != "X" ]; then
  set -x
fi

# makesrpm.sh - generates rpms from the currently checked out HEAD,
# along with condor.spec and the sources in this directory

usage () {
  echo "usage: $(basename "$0") [-ba|-bs]"
  echo "  -ba    Build binary and source packages"
  echo "  -bs    Build source package only (default)"
  exit
}

case $1 in
   -ba | -bs ) buildmethod=$1 ;;
          '' ) buildmethod=-bs ;;
  --help | * ) usage ;;
esac

# Do everything in a temp dir that will go away on errors or end of script
tmpd=$(mktemp -d -p $PWD .tmpXXXXXX)
trap 'rm -rf "$tmpd"' EXIT

pushd "$(dirname "$0")"   >/dev/null  # go to srpm dir
pushd ../../..            >/dev/null  # go to root of git tree

# why is it so hard to do a "git cat" ?
condor_version=$( git archive HEAD CMakeLists.txt | tar xO \
                  | awk -F\" '/^set\(VERSION / {print $2}' )

git archive HEAD | gzip > "$tmpd/condor.tar.gz"

git_rev=$(git log -1 --pretty=format:%h)

popd >/dev/null # back to srpm dir or initial dir.

# should verify this: [[ $condor_version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]

mkdir "$tmpd/SOURCES"
cp -p -- * "$tmpd/SOURCES/"
mv "$tmpd/condor.tar.gz" "$tmpd/SOURCES/"

sed -i "
  s/^%define git_rev .*/%define git_rev $git_rev/
  s/^%define tarball_version .*/%define tarball_version $condor_version/
" "$tmpd/SOURCES/condor.spec"

rpmbuild $buildmethod -D"_topdir $tmpd" "$tmpd/SOURCES/condor.spec"

popd >/dev/null  # back to original working dir
mv "$tmpd"/*RPMS/*.rpm .

