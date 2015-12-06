# Programming Guidelines

### Coding Style
We use [Linux Kernel](https://www.kernel.org/doc/Documentation/CodingStyle)
style with 4 spaces indentation.
### Workflow
We use [git-flow](http://jeffkreeftmeijer.com/2010/why-arent-you-using-git-flow/) as development model.

Steps:
 * Open github issue, say it's #9
 * Create feature branch named as issue's number
   `git flow feature start 9`
 * Develop and commit. Every commit message should reference the issue, eg
   include `[#9]` text. See [Note2](#note2) for helpfull stuff
 * Finish feature `git flow feature finish`
 * Close github issue **manually**. See [Note1](#note1) for explanation.
 * Update changelog with [git-changelog](https://github.com/aanatoly/git-changelog)
 * Push it

### Release Procedure

**Do locally**

```bash
VER=7.0
git flow release start $VER
# update version
vi version
# update changelog
git-changelog CHANGELOG.md
git commit -a
git flow release finish
git push --all
git push --tags
```


**Do at [Github](https://github.com/)**

Go to project's [Releases](https://github.com/aanatoly/fbpanel/releases) page and turn the `$VER` tag into release.

-------

##### note1
Github delays closing of the issue, until commit with `closes #NN` merges into
`master`. Which in our case happends quite rare. So to make issue tracker
reflect real situation, we close it manually.

##### note2
To automatically insert issue refernce into commit message, you may
install [chaining-git-hooks](https://github.com/aanatoly/chaining-git-hooks)
with [prepare-commit-msg-01-ref-issues](https://github.com/aanatoly/chaining-git-hooks/blob/master/prepare-commit-msg-01-ref-issues) hook.
