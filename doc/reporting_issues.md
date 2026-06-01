# 8.1 How to create bug reports via GitHub Issues

If you’ve found something wrong with the server emulator, whether it’s a bug, an exploit, or anything else, thank you for taking the time to report it! This helps improve the project for everyone. Please read below to make sure your report is clear and useful.

## 8.1.1 Types of Issues

### Bugs
Something doesn’t work as expected, maybe a feature is broken, or there's an error that crashes part of the server. If you didn’t expect it, it’s probably a bug.

### Exploits
These are security issues or unintended mechanics that can be abused by players. These are especially important to report in detail.

### Others
This can include performance issues, weird behaviors, missing features that *should* be there, or anything else you think is worth mentioning.


## 8.1.2 How to Report an Issue
Please open an issue here on GitHub and include the following:

### Target
Tell us where the issue happens:
- `Main Server`
- `Cast Server`
- `Auth Server`
- `Common` (shared code or logic used by multiple servers)

### File Path
Point to the exact file and line (if possible). For example:
`src/AuthServer/LoginHandler.cpp`

### What’s the Issue?
Describe clearly what’s going wrong. Include:
- What you expected to happen
- What actually happened
- Any logs, screenshots, or relevant outputs

### How to Reproduce
Explain step-by-step how we can trigger the problem. The simpler, the better. Example:
1. Connect to the Auth Server with empty credentials
2. Server crashes with `NullPointerException`

### Optional: Operating System
The emulator may have issues only on Linux, or perhaps only on Windows. If that's the case, let us know.


## Found a Fix? Feel free to open a PR!

If you’ve already found a fix that's even better! 
In that case, **please open a Pull Request (PR)** along with your issue so we can review and merge it.

Be sure your PR references the issue number (e.g. `Fixes #42`) so everything stays linked together.


### Thanks again for helping improve the project! 


## Next
[9.1 Credits and community contributions](https://github.com/DownWithTheFallen/MicrovoltsEmulator/blob/mv1.1_2.0/doc/thanks.md)
