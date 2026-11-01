# GitHub Actions workflows (manual only)

The owner's rule (#141): *"we are not going to have any github actions that
run automatically... manual invocation only"*.

Every workflow in this folder therefore:

- has **`workflow_dispatch` as its only trigger**. There are no `push`,
  `pull_request`, `schedule`, `workflow_run`, `release` or other event
  triggers, not even for a lint-only job. A platform workflow may also have
  `workflow_call` next to it, so the
  manually dispatched `all.yml` can run it; `workflow_call` alone is rejected.
- sets top-level **`permissions: contents: read`**, and no job grants write;
- **pins every action to a full 40-character commit SHA**, with the version in
  a comment, GitHub's own `actions/*` included:
  `uses: actions/checkout@<sha>  # v4.2.2`. Local `./` actions are fine.

`tools/check_workflows.py` enforces all three. Run it before every PR that
touches `.github/`, and in release checks:

```bash
python3 tools/check_workflows.py              # every .github/workflows/*.yml / *.yaml
python3 tools/check_workflows.py --self-test  # the checker's own good/bad samples
```

It exits 0 when every workflow passes (or there are none) and prints one line
per problem otherwise. It needs no third-party Python packages.

## Who starts a workflow

Only someone with the owner's explicit consent, given in the moment for that
run: which workflow, which ref, which inputs. Agents never start one on their
own.

## Running a workflow

A workflow file has to exist on the **default branch (`main`)** before it can
be dispatched for the first time. Once it's there, you can run it against any
branch that has the file:

```bash
gh workflow list -R homer6/cpp-docs
gh workflow run <file>.yml -R homer6/cpp-docs --ref <branch> -f key=value
```

`-f key=value` sets one `workflow_dispatch` input; repeat it for more.

## Watching and reading a run

```bash
gh run list -R homer6/cpp-docs --workflow <file>.yml   # find the run ID
gh run watch <run-id> -R homer6/cpp-docs                # follow it live
gh run view <run-id> -R homer6/cpp-docs                 # summary and jobs
gh run view <run-id> -R homer6/cpp-docs --log-failed    # logs of the failed steps
gh run download <run-id> -R homer6/cpp-docs -D <dir>    # artifacts it uploaded
```

A run's result is evidence to verify, like any other output. It doesn't
replace the local checks in `CLAUDE.md` ("Running the tests").
