using System.Numerics;
using System.Security.Cryptography.X509Certificates;
using System.Text.Json.Serialization;
using In3.Utils;

namespace In3.Eth1
{
    public class Transaction
    {
        [JsonPropertyName("blockHash")] public string BlockHash { get; set; }
        [JsonPropertyName("blockNumber"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger BlockNumber { get; set; }
        [JsonPropertyName("chainId")] public string ChainId { get; set; }
        [JsonPropertyName("creates")] public string Creates { get; set; }
        [JsonPropertyName("from")] public string From { get; set; }
        [JsonPropertyName("gas"), JsonConverter(typeof(LongFromHexConverter))] public long Gas { get; set; }
        [JsonPropertyName("gasPrice"), JsonConverter(typeof(LongFromHexConverter))] public long GasPrice { get; set; }
        [JsonPropertyName("hash")] public string Hash { get; set; }
        [JsonPropertyName("input")] public string Input { get; set; }
        [JsonPropertyName("nonce"), JsonConverter(typeof(LongFromHexConverter))] public long Nonce { get; set; }
        [JsonPropertyName("publicKey")] public string PublicKey { get; set; }
        [JsonPropertyName("r")] public string R { get; set; }
        [JsonPropertyName("raw")] public string Raw { get; set; }
        [JsonPropertyName("s")] public string S { get; set; }
        [JsonPropertyName("standardV")] public string StandardV { get; set; }
        [JsonPropertyName("to")] public string To { get; set; }
        [JsonPropertyName("transactionIndex")] public string TransactionIndex { get; set; }
        [JsonPropertyName("v")] public string V { get; set; }
        [JsonPropertyName("value"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger Value { get; set; }
    }
}