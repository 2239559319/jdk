/*
 * Copyright (c) 2015, 2023, Oracle and/or its affiliates. All rights reserved.
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

/**
 * Defines the HTTP Client and WebSocket APIs.
 * <p>
 * <b id="httpclientprops">System properties used by the java.net.http API</b>
 * <p>
 * The following is a list of system networking properties used by the java.net.http
 * client implementation in the JDK. Any properties below that take a numeric value
 * assume the default value if given a string that does not parse as a number.
 * Unless otherwise specified below, all values can be set in the {@code conf/net.properties}
 * file. In all cases, values can be specified as system properties on the command line,
 * in which case, any value in {@code conf/net.properties} is overridden. No guarantee is
 * provided that property values can be set programatically with {@code System.setProperty()}.
 * Other implementations of this API may choose not to support these properties.
 * <ul>
 * <li><p><b>{@systemProperty jdk.httpclient.allowRestrictedHeaders}</b> (default: see below)<br>
 * A comma-separated list of normally restricted HTTP header names that users may set in HTTP
 * requests or by user code in HttpRequest instances. By default, the following request
 * headers are not allowed to be set by user code: connection, content-length, expect, host,
 * and upgrade. You can override this behavior with this property. The names are case-insensitive
 * and whitespace is ignored. Note that this property is intended for testing and not for
 * real-world deployments. Protocol errors or other undefined behavior are likely to occur
 * when using this property. There may be other headers that are restricted from being set
 * depending on the context. This includes the "Authorization" header when the relevant
 * HttpClient has an authenticator set. These restrictions cannot be overridden by this
 * property.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.bufsize}</b> (default: 16384 bytes or 16 kB)<br>
 * The size to use for internal allocated buffers in bytes.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.connectionPoolSize}</b> (default: 0)<br>
 * The maximum number of connections to keep in the HTTP/1.1 keep alive cache. A value of 0
 * means that the cache is unbounded.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.connectionWindowSize}</b> (default: 2^26)<br>
 * The HTTP/2 client connection window size in bytes. The maximum size is 2^31-1. This value
 * cannot be smaller than the stream window size, which can be configured through the
 * {@code jdk.httpclient.windowsize} system property.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.disableRetryConnect}</b> (default: false)<br>
 * Whether automatic retry of connection failures is disabled. If false, then retries are
 * attempted (subject to the retry limit).
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.enableAllMethodRetry}</b> (default: false)<br>
 * Whether it is permitted to automatically retry non-idempotent HTTP requests.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.enablepush}</b> (default: 1)<br>
 * Whether HTTP/2 push promise is enabled. A value of 1 enables push promise; a value of 0
 * disables it.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.hpack.maxheadertablesize}</b> (default: 16384 or
 * 16 kB)<br> The HTTP/2 client maximum HPACK header table size in bytes.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.qpack.decoderMaxTableCapacity}</b> (default: 0)
 * <br> The HTTP/3 client maximum QPACK decoder dynamic header table size in bytes.
 * <br> Setting this value to a positive number will allow HTTP/3 servers to add entries
 * to the QPack decoder's dynamic table. When set to 0, servers are not permitted to add
 * entries to the client's QPack encoder's dynamic table.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.qpack.encoderTableCapacityLimit}</b> (default: 4096,
 * or 4 kB)
 * <br> The HTTP/3 client maximum QPACK encoder dynamic header table size in bytes.
 * <br> Setting this value to a positive number allows the HTTP/3 client's QPack encoder to
 * add entries to the server's QPack decoder's dynamic table, if the server permits it.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.HttpClient.log}</b> (default: none)<br>
 * Enables high-level logging of various events through the {@linkplain java.lang.System.Logger
 * Platform Logging API}. The value contains a comma-separated list of any of the
 * following items:
 * <ul>
 *   <li>errors</li>
 *   <li>requests</li>
 *   <li>headers</li>
 *   <li>content</li>
 *   <li>frames</li>
 *   <li>ssl</li>
 *   <li>trace</li>
 *   <li>channel</li>
 *   <li>http3</li>
 *   <li>quic</li>
 * </ul><br>
 * You can append the frames item with a colon-separated list of any of the following items:
 * <ul>
 *   <li>control</li>
 *   <li>data</li>
 *   <li>window</li>
 *   <li>all</li>
 * </ul><br>
 * You can append the quic item with a colon-separated list of any of the following items;
 * packets are logged in an abridged form that only shows frames offset and length,
 * but not content:
 * <ul>
 *   <li>ack: packets containing ack frames will be logged</li>
 *   <li>cc: information on congestion control will be logged</li>
 *   <li>control: packets containing quic controls (such as frames affecting
 *                flow control, or frames opening or closing streams)
 *                will be logged</li>
 *   <li>crypto: packets containing crypto frames will be logged</li>
 *   <li>data: packets containing stream frames will be logged</li>
 *   <li>dbb: information on direct byte buffer usage will be logged</li>
 *   <li>ping: packets containing ping frames will be logged</li>
 *   <li>processed: information on flow control (processed bytes) will be logged</li>
 *   <li>retransmit: information on packet loss and recovery will be logged</li>
 *   <li>timer: information on send task scheduling will be logged</li>
 *   <li>all</li>
 * </ul><br>
 * Specifying an item adds it to the HTTP client's log. For example, if you specify the
 * following value, then the Platform Logging API logs all possible HTTP Client events:<br>
 * "errors,requests,headers,frames:control:data:window,ssl,trace,channel"<br>
 * Note that you can replace control:data:window with all. The name of the logger is
 * "jdk.httpclient.HttpClient", and all logging is at level INFO.
 * To debug issues with the quic protocol a good starting point is to specify
 * {@code quic:control:retransmit}.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.keepalive.timeout}</b> (default: 30)<br>
 * The number of seconds to keep idle HTTP connections alive in the keep alive cache. This
 * property applies to both HTTP/1.1 and HTTP/2. The value for HTTP/2 can be overridden
 * with the {@code jdk.httpclient.keepalive.timeout.h2 property}.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.keepalive.timeout.h2}</b> (default: see
 * below)<br>The number of seconds to keep idle HTTP/2 connections alive. If not set, then the
 * {@code jdk.httpclient.keepalive.timeout} setting is used.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.keepalive.timeout.h3}</b> (default: see
 * below)<br>The number of seconds to keep idle HTTP/3 connections alive. If not set, then the
 * {@code jdk.httpclient.keepalive.timeout.h2} setting is used.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.maxframesize}</b> (default: 16384 or 16kB)<br>
 * The HTTP/2 client maximum frame size in bytes. The server is not permitted to send a frame
 * larger than this.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.maxstreams}</b> (default: 100)<br>
 * The maximum number of HTTP/2 push streams that the client will permit servers to open
 * simultaneously.
 * </li>
 * <li><p><b>{@systemProperty jdk.http3.maxConcurrentPushStreams}</b> (default: 100)<br>
 * The maximum number of HTTP/3 push streams that the client will permit servers to open
 * simultaneously.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.receiveBufferSize}</b> (default: operating system
 * default)<br>The HTTP client <a href="../java.base/java/net/StandardSocketOptions.html#SO_RCVBUF">
 * socket receive buffer size</a> in bytes.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.redirects.retrylimit}</b> (default: 5)<br>
 * The maximum number of attempts to send a HTTP request when redirected or any failure occurs
 * for any reason.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.websocket.writeBufferSize}</b> (default: 16384
 * or 16kB)<br>The buffer size used by the web socket implementation for socket writes.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.windowsize}</b> (default: 16777216 or 16 MB)<br>
 * The HTTP/2 client stream window size in bytes.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.auth.retrylimit}</b> (default: 3)<br>
 * The number of attempts the Basic authentication filter will attempt to retry a failed
 * authentication.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.sendBufferSize}</b> (default: operating system
 * default)<br>The HTTP client socket
 * <a href="../java.base/java/net/StandardSocketOptions.html#SO_SNDBUF">send buffer size</a>.
 * Values less than or equal to zero are ignored.
 * </li>
 * <li><p><b>{@systemProperty jdk.internal.httpclient.disableHostnameVerification}</b> (default:
 * false)<br>If true (or set to an empty string), hostname verification in SSL certificates
 * is disabled. This is a system property only and not available in {@code conf/net.properties}.
 * It is provided for testing purposes only.
 * </li>
 * <li><p><b>{@systemProperty jdk.http.auth.proxying.disabledSchemes}</b> (default: see
 * conf/net.properties)<br>A comma separated list of HTTP authentication scheme names,
 * that are disallowed for use by the HTTP client implementation, for HTTP proxying.
 * </li>
 * <li><p><b>{@systemProperty jdk.http.auth.tunneling.disabledSchemes}</b> (default: see
 * conf/net.properties)<br>A comma separated list of HTTP authentication scheme names, that
 * are disallowed for use by the HTTP client implementation, for HTTP CONNECT tunneling.
 * </li>
 * </ul>
 * The following system properties can be used to configure some aspects of the
 * <a href="https://www.rfc-editor.org/info/rfc9000">QUIC Protocol</a>
 * implementation used for HTTP/3:
 * <ul>
 * <li><p><b>{@systemProperty jdk.httpclient.quic.receiveBufferSize}</b> (default: operating system
 * default)<br>The QUIC {@linkplain java.nio.channels.DatagramChannel UDP client socket}
 * <a href="../java.base/java/net/StandardSocketOptions.html#SO_RCVBUF">
 * socket receive buffer size</a> in bytes.
 * Values less than or equal to zero are ignored.
 * </li>
 * <li><p><b>{@systemProperty jdk.httpclient.quic.sendBufferSize}</b> (default: operating system
 * default)<br>The QUIC {@linkplain java.nio.channels.DatagramChannel UDP client socket}
 * <a href="../java.base/java/net/StandardSocketOptions.html#SO_SNDBUF">send buffer size</a>.
 * Values less than or equal to zero are ignored.
 * </li>
 *
 * </ul>
 * <blockquote>
 *     // TODO: Revisit - there are more properties we may want to expose and
 *     //       and document for HTTP/3, QPack, and Quic related settings
 </blockquote>
 * @moduleGraph
 * @since 11
 */
module java.net.http {
    exports java.net.http;
}
