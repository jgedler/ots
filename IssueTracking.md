# Bug reporting guidelines #

For the time being, we don't use our own issue tracking site. Instead, you can use Chromium project's tracking system at http://crbug.com/. When you report a bug on the site, please add a string "OTS" to your bug description (example: http://crbug.com/27131).

You might want to read this article before submitting a bug report: http://dev.chromium.org/for-testers/bug-reporting-guidelines.

# Known issues #

http://code.google.com/p/chromium/issues/list?can=2&q=OTS

# Unsupported fonts #

Fonts in http://code.google.com/p/ots/source/browse/trunk/test/BLACKLIST.txt are known to trigger parser errors. ots doesn't produce a transcoded font for these fonts.