# wtb - print currently open bug ID

## DESCRIPTION

what the bug - print currently open bug ID on your X

## REQUIREMENTS

* gcc
* make
* Xlib header files

## INSTALLATION

Installed by default into /usr/local (change PREFIX for custom prefix)

    PREFIX=/usr make install

## USING

```
    git branch fix-bug-$(wtb)
    git commit -m "Fix ... Closes #$(wtb)"
```

## WHY?

This is Perl to C translation of:
http://people.gnome.org/~tthurman/metacity/bugid.txt

Adding support for other BTS:

* Trac
* Redmine
* Debian BTS
* MantisBT
* Bugzilla
* Jira
* Flyspray
* PHP Bug Tracking System
* Github Issues
* GitLab Issues

## LICENSE

MIT/X Consortium License

© 2011-2016 Laurent Arnoud <laurent@spkdev.net>
