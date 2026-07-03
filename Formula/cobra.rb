class Cobra < Formula
  desc "Indentation-based programming language compiled to x86-64 native"
  homepage "https://github.com/Xznder1984/Cobra"
  url "https://github.com/Xznder1984/Cobra/releases/download/v0.1.0/cobra-0.1.0-macos-x86_64.tar.gz"
  sha256 "6d30433f2525b69f74f9dfe5a8614c813eae4fdee1febe1469f822187ae4fc09"

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
