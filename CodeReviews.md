For now, we don't use our own code review system. Instead, you can use Chromium project's Rietveld server (http://codereview.chromium.org/). Here is the instruction how to use the server:

  1. checkout Chromium source code (http://dev.chromium.org/developers).
    1. install depot\_tools.
    1. checkout Chromium code from their SVN server.
    1. run "gclient sync" to pull OTS code into chromium/src/third\_party/ots/.
  1. modify OTS code under chromium/src/third\_party/ots/.
    * `cd chromium/src/third_party/ots/`
    * `emacs src/cmap.cc`   # for example
  1. start code review using gcl (http://dev.chromium.org/developers/contributing-code). Please assign an OTS project member (e.g. yusukes@chromium.org) as a reviewer.
    * `cd chromium/src/third_party/ots/`
    * `gcl change your_change_name`   # you can safely ignore the "WARNING: Creating CL in a read-only checkout" warning.
    * `gcl upload your_change_name`
  1. after the code review, the reviewer will land your patch.