#!/bin/bash
# Copyright 2017-2022 Matt "MateoConLechuga" Waltz
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

../bin/convbin --iformat bin --input inputs/random_64k.bin --oformat 8xp --output THEISTE.8xp --name TESTTEST || { exit 1; }
../bin/convbin --iformat bin --input inputs/random_128k.bin --oformat 8xp --output THEISTE.8xp --name TESTTEST || { exit 1; }
../bin/convbin --iformat 8x --input inputs/demo.8xp --input inputs/fileioc.8xv --input inputs/libload.8xv --oformat 8xg-auto-extract --output test.8xg.test || { exit 1; }
../bin/convbin --iformat 8x --input inputs/demo.8xp --oformat bin --output test.bin.test || { exit 1; }
../bin/convbin --iformat 8x --input inputs/fileioc.8xv --oformat bin --output test.bin.test || { exit 1; }
../bin/convbin --iformat bin --input inputs/large.bin --oformat c --output test.c.test --name TEST || { exit 1; }
../bin/convbin --iformat bin --input inputs/large.bin --oformat asm --output test.asm.test --name TEST || { exit 1; }
../bin/convbin --iformat 8x --input inputs/demo.8xp --oformat ice --output test.ice.test --name TEST || { exit 1; }
../bin/convbin --iformat 8x --input inputs/demo.8xp --oformat 8xp --output test.8xp.test --name TEST || { exit 1; }
../bin/convbin --iformat bin --input inputs/small.bin --oformat 8xp --output test.8xp.test --name TEST || { exit 1; }
../bin/convbin --iformat 8x --input inputs/demo.8xp --oformat 8xp-compressed --output test.8xp --name TEST || { exit 1; }
../bin/convbin --iformat 8x --input inputs/demo.8xp -e zx0 --oformat 8xp-compressed --output test.8xp --name TEST || { exit 1; }
../bin/convbin --iformat bin --input inputs/small.bin --oformat c --output test.c.test  --name TEST --compress zx7 || { exit 1; }
../bin/convbin --iformat bin --input inputs/large.bin --oformat asm --output test.asm.test --name TEST --compress zx7 || { exit 1; }
../bin/convbin --iformat 8x --input inputs/demo.8xp --oformat ice --output test.ice.test --name TEST --compress zx7 || { exit 1; }
../bin/convbin --iformat 8x --input inputs/libload.8xv --oformat 8xp --output test.8xp.test --name TEST --compress zx7 || { exit 1; }
../bin/convbin --iformat csv --input inputs/csv.csv --oformat c --output test.c.test --name TEST || { exit 1; }
../bin/convbin --iformat csv --input inputs/csv.csv --oformat asm --output test.asm.test --name TEST || { exit 1; }
../bin/convbin --iformat csv --input inputs/csv.csv --oformat ice --output test.ice.test --name TEST || { exit 1; }
../bin/convbin --iformat csv --input inputs/csv.csv --oformat 8xp --output test.8xp.test --name TEST || { exit 1; }
../bin/convbin --iformat bin --input inputs/small.bin --oformat c --output test.8xp.test --name super_long_name_that_apparently_people_like_to_use_for_their_variable_names_even_though_it_is_basically_unreadable_but_hey_who_cares_the_user_gets_what_the_user_wants || { exit 1; }
