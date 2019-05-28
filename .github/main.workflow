workflow "Build branch" {
  on = "push"
  resolves = ["GitHub Action for Google Cloud"]
}

action "GitHub Action for Google Cloud SDK auth" {
  uses = "actions/gcloud/auth@ba93088eb19c4a04638102a838312bb32de0b052"
  secrets = ["GOOGLE_CREDENTIALS"]
}

action "GitHub Action for Google Cloud" {
  uses = "actions/gcloud/cli@ba93088eb19c4a04638102a838312bb32de0b052"
  needs = ["GitHub Action for Google Cloud SDK auth"]
  args = "builds submit -t eu.gcr.io/domotique-201311/tassimo-build"
}