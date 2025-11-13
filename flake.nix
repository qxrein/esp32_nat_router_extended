
{
  description = "PlatformIO development environment for ESP/Arduino projects";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          name = "platformio-env";

          buildInputs = with pkgs; [
            python3
            platformio
            platformio-core
            git
          ];

          # Optional tools for embedded dev
          nativeBuildInputs = with pkgs; [
            pkgs.gcc
            pkgs.gdb
            pkgs.esptool
          ];

          shellHook = ''
            echo "🛠️  PlatformIO environment ready!"
            echo "Type 'pio run' to build or 'pio run -t upload' to flash."
          '';
        };
      });
}


