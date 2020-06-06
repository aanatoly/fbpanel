# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

inherit toolchain-funcs
inherit subversion

ESVN_REPO_URI="https://fbpanel.svn.sourceforge.net/svnroot/fbpanel/trunk"
ESVN_PROJECT="fbpanel"

DESCRIPTION="light-weight X11 desktop panel"
HOMEPAGE="http://fbpanel.sourceforge.net/"

LICENSE="GPL"
SLOT="0"
KEYWORDS="~arm alpha amd64 ppc ~ppc64 x86"
IUSE=""

RDEPEND=">=x11-libs/gtk+-2"
DEPEND="${RDEPEND}
	dev-util/pkgconfig"


src_install() {
	emake DESTDIR="${D}" install || die "Install failed"
	dodoc CHANGELOG CREDITS README
}

