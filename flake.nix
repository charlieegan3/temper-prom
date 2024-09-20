{
  description = "Nix flake for the temper-prom project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "temper-prom";
          version = "1.0.0";

          src = ./.;

          buildInputs = [
            pkgs.pkg-config
            pkgs.libmicrohttpd
            pkgs.libusb
            pkgs.gcc
          ];

          buildPhase = "make";

          installPhase = ''
            mkdir -p $out/bin
            cp temper-prom $out/bin/
          '';

          meta = with pkgs.lib; {
            description = "A temperature monitoring tool using TEMPer USB sticks and exposing data via HTTP.";
            platforms = platforms.all;
          };
        };

        apps.temper-prom = {
          type = "app";
          program = "${self.packages.${system}.temper-prom}/bin/temper-prom";
        };
      }
    );
}
