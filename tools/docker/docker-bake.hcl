variable "tag" { default = "23.3.2" }
variable "opentxs" { default = "1.189.0" }

variable "target_arch" {
  default = "amd64"
}
variable "repo" {
  default = "https://github.com/Open-Transactions/metier-server"
}

variable "sdk" {
  default = "opentransactions/alpine-sdk"
}

variable "runtime" {
  default = "opentransactions/alpine-runtime"
}

target "downloader" {
  dockerfile = "downloader"
  target = "downloader"
  tags = [
    "temp-metier-server/downloader"
  ]
}

target "metier-server-download" {
  dockerfile = "download"
  target = "metier-server-download"
  ssh = [ "default" ]
  contexts = {
    download = "target:downloader"
  }
  args = {
    METIER_SERVER_REPO = "${repo}"
    METIER_SERVER_TAG = "${tag}"
  }
  tags = [
    "temp-metier-server/metier-server-download"
  ]
}

target "compile" {
  dockerfile = "compile"
  target = "compile"
  contexts = {
    metier-server-download = "target:metier-server-download"
  }
  args = {
    OPENTXS_SDK = "${sdk}"
    OPENTXS_VERSION = "${opentxs}"
  }
  tags = [
    "temp-metier-server/compile"
  ]
}

target "default" {
  dockerfile = "run"
  target = "run"
  contexts = {
    compile = "target:compile"
  }
  args = {
    OPENTXS_RUNTIME = "${runtime}"
    OPENTXS_VERSION = "${opentxs}"
  }
  tags = [
    "opentransactions/metier-server:${tag}-${opentxs}-${target_arch}",
    "opentransactions/metier-server:latest-${target_arch}"
  ]
}
