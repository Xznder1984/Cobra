class Cobra < Formula
  desc "Indentation-based programming language compiled to x86-64 native"
  homepage "https://github.com/Xznder1984/Cobra"
  url "https://github.com/Xznder1984/Cobra/releases/download/v0.1.0/cobra-0.1.0-macos-x86_64.tar.gz"
  sha256 "PLACEHOLDER"

  depends_on "make" => :build

  def install
    bin.install "cobra"
    bin.install "cobrac"
    lib.install "libcobra_runtime.a"
  end

  test do
    system "#{bin}/cobra", "--version"
  end
end
