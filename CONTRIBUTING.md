## e2sim Contribution Guidelines

This file is intended to provide some guidelines on how to contribute to this project.

Unfortunatelly, github does not allow forking a public repository to a private one. Thus, we duplicated the official repository from the O-RAN SC into this one.
This strategy still allows to update the base source code from the O-RAN SC repository while merging our changes on top of it into our private repository.

The following steps were employed after creating this repo:
1. This step is just for documentation (_not required to run again_), please go to the next step:
```
git clone --bare https://github.com/o-ran-sc/sim-e2-interface.git
cd sim-e2-interface.git
git push --mirror <url-of-this-repo>/e2sim.git
```

2. Clone our repository e make changes as follows:
```
git clone git@github.com:LABORA-INF-UFG/e2sim.git my-e2sim
cd my-e2sim
git checkout -b my-patch-branch # create and switch to the new branch
# make your changes
git commit
git push origin my-patch-branch
# open a git pull request
```
To open a pull request please check the [Create a pull request](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request) page on GitHub.

3. When an update of the base source code (from the official O-RAN SC repo) is required, do the following:
```
cd  my-e2sim
git remote add oran-public-mirror https://github.com/o-ran-sc/sim-e2-interface.git
git pull oran-public-mirror master # will create a merge commit
git push origin master
```

