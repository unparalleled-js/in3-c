/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

package in3;

import in3.Proof;
import in3.StorageProvider;
import in3.config.ClientConfiguration;
import in3.config.Configuration;
import in3.eth1.API;

/**
 * This is the main class creating the incubed client. The client can then be
 * configured.
 *
 */
public class IN3 {

    static {
        Loader.loadLibrary();
    }

    private long ptr;
    private StorageProvider provider;
    private Signer signer;

    private static IN3Transport transport = new IN3DefaultTransport();
    private ClientConfiguration config;

    @Deprecated
    public IN3() {
        ptr = init(0);
    }

    private IN3(long chainAlias) {
        ptr = init(chainAlias);
    }

    /**
     * create a Incubed client using the chain-config.
     * if chainId is Chain.MULTICHAIN, the client can later be switched between different chains,
     * for all other chains, it will be initialized only with the chainspec for this one chain (safes memory)
     */
    public static IN3 forChain(long chainId) {
        return new IN3(chainId);
    }

    protected void finalize() {
        free();
    }

    /** sets config object in the client */
    private native void setConfig(String val);

    /** the client key to sign requests */
    public native byte[] getKey();

    /** sets the client key to sign requests */
    public native void setKey(byte[] val);

    protected void setConfig(ClientConfiguration config) {
        this.config = config;
    }

    protected void applyConfig() {
        setConfig(config.toJSON());
        config.markAsSynced();
    }

    /**
     * returns the current configuration.
     * any changes to the configuration will be applied witth the next request.
     */
    public ClientConfiguration getConfig() {
        return config;
    }

    /** sets the client key as hexstring to sign requests */
    public void setKey(String val) {
        if (val == null)
            setKey((byte[]) null);
        else {
            byte[] key = new byte[32];
            for (int i = 0; i < 32; i++)
                key[i] = (byte) ((Character.digit(val.charAt(i * 2 + 2), 16) << 4)
                        | Character.digit(val.charAt(i * 2 + 3), 16));
            setKey(key);
        }
    }

    /**
     * sets the signer or wallet.
     */
    public void setSigner(Signer signer) {
        this.signer = signer;
    }

    /**
     * returns the signer or wallet.
     */
    public Signer getSigner() {
        return signer;
    }

    /**
     * gets the ethereum-api
     */
    public in3.eth1.API getEth1API() {
        return new API(this);
    }

    /**
     * provides the ability to cache content like nodelists, contract codes and
     * validatorlists
     */
    public void setStorageProvider(StorageProvider val) {
        provider = val;
        initcache();
    }

    /** provides the ability to cache content */
    public StorageProvider getStorageProvider() {
        return provider;
    }

    /**
     * sets The transport interface.
     * This allows to fetch the result of the incubed in a different way.
     */
    public void setTransport(IN3Transport newTransport) {
        IN3.transport = newTransport;
    }

    /**
     * returns the current transport implementation.
     */
    public IN3Transport getTransport() {
    	return IN3.transport;
    }

    /** servers to filter for the given chain. The chain-id based on EIP-155. */
    public native long getChainId();

    /** sets the chain to be used. The chain-id based on EIP-155. */
    public native void setChainId(long val);

    /**
     * send a request. The request must a valid json-string with method and params
     */
    public native String send(String request);

    /**
     * send a request but returns a object like array or map with the parsed
     * response. The request must a valid json-string with method and params
     */
    public native Object sendobject(String request);

    private String toRPC(String method, Object[] params) {
        String p = "";
        for (int i = 0; i < params.length; i++) {
            if (p.length() > 0)
                p += ",";
            if (params[i] == null)
                p += "null";
            else if (params[i] instanceof String) {
                String s = (String) params[i];
                if (s.charAt(0) == '{' || s.equals("true") || s.equals("false"))
                    p += s;
                else
                    p += "\"" + s + "\"";
            } else
                p += params[i].toString();
        }
        return "{\"method\":\"" + method + "\", \"params\":[" + p + "]}";
    }

    /**
     * send a RPC request by only passing the method and params. It will create the
     * raw request from it and return the result.
     */
    public String sendRPC(String method, Object[] params) {
        if (!config.isSynced()) {
            this.applyConfig();
        }
        return this.send(toRPC(method, params));
    }

    /**
     * send a RPC request by only passing the method and params. It will create the
     * raw request from it and return the result.
     */
    public Object sendRPCasObject(String method, Object[] params) {
        if (!config.isSynced()) {
            this.applyConfig();
        }
        return this.sendobject(toRPC(method, params));
    }

    /** internal function to handle the internal requests */
    static byte[][] sendRequest(String[] urls, byte[] payload) {
        return IN3.transport.handle(urls, payload);
    }

    private native void free();

    private native long init(long chainId);

    private native void initcache();

    // Test it
    public static void main(String[] args) {
        Object[] params = new Object[args.length - 1];
        for (int i = 1; i < args.length; i++)
            params[i - 1] = args[i];

        // create client
        IN3 in3 = IN3.forChain(Chain.MAINNET);

        // set cache in tempfolder
        in3.setStorageProvider(new in3.TempStorageProvider());

        // execute the command
        System.out.println(in3.sendRPC(args[0], params));
    }
}