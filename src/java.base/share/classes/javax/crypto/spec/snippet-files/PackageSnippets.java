/*
 * Copyright (c) 2025, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
import javax.crypto.Cipher;
import javax.crypto.spec.HPKEParameterSpec;
import java.nio.charset.StandardCharsets;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.util.Arrays;

class PackageSnippets {
    public static void main(String[] args) throws Exception {
        // @start region="hpke-spec-example"
        // Recipient key pair generation
        KeyPairGenerator g = KeyPairGenerator.getInstance("X25519");
        KeyPair kp = g.generateKeyPair();

        // The HPKE sender side is initialized with the recipient's public key
        // and default HPKEParameterSpec with an application-supplied info.
        Cipher senderCipher = Cipher.getInstance("HPKE");
        HPKEParameterSpec ps = HPKEParameterSpec.of()
                .info("app_info".getBytes(StandardCharsets.UTF_8));
        senderCipher.init(Cipher.ENCRYPT_MODE, kp.getPublic(), ps);

        // Retrieve the actual parameters used from the sender.
        HPKEParameterSpec actual = senderCipher.getParameters()
                .getParameterSpec(HPKEParameterSpec.class);

        // Retrieve the key encapsulation message (the KEM output) from the sender.
        // It can also be retrieved using sender.getIV().
        byte[] kemEncap = actual.encapsulation();

        // The HPKE recipient side is initialized with its own private key,
        // the same algorithm identifiers as used by the sender,
        // and the key encapsulation message from the sender.
        Cipher recipientCipher = Cipher.getInstance("HPKE");
        HPKEParameterSpec pr = HPKEParameterSpec
                .of(actual.kem_id(), actual.kdf_id(), actual.aead_id())
                .info("app_info".getBytes(StandardCharsets.UTF_8))
                .encapsulation(kemEncap);
        recipientCipher.init(Cipher.DECRYPT_MODE, kp.getPrivate(), pr);

        // Encryption and decryption
        byte[] msg = "Hello World".getBytes(StandardCharsets.UTF_8);
        byte[] ct = senderCipher.doFinal(msg);
        byte[] pt = recipientCipher.doFinal(ct);

        assert Arrays.equals(msg, pt);
        // @end
    }
}
