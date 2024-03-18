#if 0
The Open Speech Repository
http://www.voiptroubleshooter.com/open_speech/american.html
http://www.voiptroubleshooter.com/open_speech/british.html

/* Get the time in MS. */
//TickType_t currentTime = pdTICKS_TO_MS( xTaskGetTickCount() );

/* Delay for 1000 MS. */
//vTaskDelay( pdMS_TO_TICKS( 1000U ) );
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <esp_private/esp_clk.h>
#include <esp_vfs_fat.h>

#include <driver/sdmmc_host.h>
#include <sdmmc_cmd.h>

#include <format_wav.h>

#include <speex/speex.h>

#define FRAME_SIZE   160
#define PACKAGE_SIZE 64
#define PACKAGE_SIZE_DEC 15 // 10 // 62 // 46 // 38
//      quality           2 //  1 // 10 //  9 //  8

esp_cpu_cycle_count_t millis() {
    const int cpu_ticks_per_s = esp_clk_cpu_freq() / 1000;
    return (esp_cpu_get_cycle_count() / cpu_ticks_per_s);
}

void speex_encode_(const char* fin_name, const char* fout_name, uint32_t quality, size_t *sample_size, size_t *enc_frame_size)
{
    printf("SPEEX: Start encode %s to %s\n", fin_name, fout_name);

    int16_t in[FRAME_SIZE];
    char buf[PACKAGE_SIZE];

    // Create a new encoder state in narrowband mode
    //printf("speex_encoder_init()\n");
    void *encoder_state = speex_encoder_init(&speex_nb_mode);

    // Set quality
    speex_encoder_ctl(encoder_state, SPEEX_SET_QUALITY, &quality);

    FILE *fin = fopen(fin_name, "r");
    if (!fin)
    {
        printf("Could not open %s\n", fin_name);
        while (1) {}
    }
    FILE *fout = fopen(fout_name, "w");

    /*
    fseek(fin, 0, SEEK_END);
    *sample_size = ftell(fin) - sizeof(wav_header_t);
    fseek(fin, 0, SEEK_SET);
    printf("sample_size:%d\n", *sample_size);
    */

    wav_header_t wav_header;
    fread(&wav_header, 1, sizeof(wav_header), fin);
    print_wav_header(&wav_header);

    // Initialization of the structure that holds the bits
    SpeexBits bits;
    //printf("speex_bits_init()\n");
    speex_bits_init(&bits);

    int nbytes = speex_bits_nbytes(&bits);
    //printf("speex_bits_nbytes:nbytes:%d\n", nbytes);

    TickType_t tick2_tick1_ = 0;
    TickType_t tick2_tick1 = 0;
    printf("speex_encode:tick2 - tick1:");

    size_t enc_frame_size_ = 0;
    while (!feof(fin))
    {
        size_t frame_size = fread(in, 2, FRAME_SIZE, fin);
        if (frame_size != FRAME_SIZE) // (feof(fin))
            break;

        TickType_t tick1 = millis();

        // Flush all the bits in the struct so we can encode a new frame
        speex_bits_reset(&bits);
        // Encode frame
        speex_encode_int(encoder_state, in, &bits);
        // Copy the bits to an array of char that can be written
        *enc_frame_size = speex_bits_write(&bits, buf, PACKAGE_SIZE);

        TickType_t tick2 = millis();
        tick2_tick1 = tick2 - tick1;
        if (tick2_tick1_ != tick2_tick1) {
            tick2_tick1_ = tick2_tick1;
            // printf("speex_encode:tick2 - tick1: %ld\n", tick2_tick1);
            printf(" %ld", tick2_tick1);
        }

        if (enc_frame_size_ == 0)
            enc_frame_size_ = *enc_frame_size;
        if (*enc_frame_size != enc_frame_size_)
            printf("enc_frame_size != enc_frame_size_: %d != %d & %d\n", *enc_frame_size, enc_frame_size_, PACKAGE_SIZE);
        // write to file
        fwrite(buf, 1, enc_frame_size_, fout);
    }
    printf("\n");
    // Destroy the decoder state
    //printf("speex_encoder_destroy()\n");
    speex_encoder_destroy(encoder_state);
    // Destroy the bit-stream struct
    //printf("speex_bits_destroy()\n");
    speex_bits_destroy(&bits);

    fclose(fout);
    fclose(fin);

    printf("SPEEX: Done encode\n");
}

void speex_decode_(const char* fin_name, const char* fout_name, size_t sample_size, size_t enc_frame_size)
{
    printf("SPEEX: Start decode %s to %s; sample_size:%d, enc_frame_size:%d\n", fin_name, fout_name, sample_size, enc_frame_size);

    FILE *fin;
    FILE *fout;

    // Holds the audio that will be written to file (16 bits per sample)
    int16_t out[FRAME_SIZE];
    char buf[PACKAGE_SIZE];

    // Create a new decoder state in narrowband mode
    //printf("speex_decoder_init(\n");
    void *decoder_state = speex_decoder_init(&speex_nb_mode);

    // Set the perceptual enhancement on
    uint32_t tmp = 1;
    //printf("speex_decoder_ctl()\n");
    speex_decoder_ctl(decoder_state, SPEEX_SET_ENH, &tmp);


    int32_t frame_size;
    speex_mode_query(&speex_nb_mode, SPEEX_MODE_FRAME_SIZE, &frame_size);
    //printf("speex_mode_query:frame_size:%ld\n", frame_size);
    int32_t bits_per_frame;
    speex_mode_query(&speex_nb_mode, SPEEX_SUBMODE_BITS_PER_FRAME, &bits_per_frame);
    //printf("speex_mode_query:bits_per_frame:%ld\n", bits_per_frame);

    speex_decoder_ctl(decoder_state, SPEEX_GET_FRAME_SIZE, &frame_size);
    printf("speex_decoder_ctl:frame_size:%ld\n", frame_size);
    int32_t enh_enabled;
    speex_decoder_ctl(decoder_state, SPEEX_GET_ENH, &enh_enabled);
    printf("speex_decoder_ctl:enh_enabled:%ld\n", enh_enabled);
    /*
    int32_t complexity;
    speex_decoder_ctl(decoder_state, SPEEX_GET_COMPLEXITY, &complexity);
    printf("speex_decoder_ctl:complexity:%ld\n", complexity);
    */
    int32_t rate;
    speex_decoder_ctl(decoder_state, SPEEX_GET_SAMPLING_RATE, &rate);
    printf("speex_decoder_ctl:rate:%ld\n", rate);
    int32_t bitrate;
    speex_decoder_ctl(decoder_state, SPEEX_GET_BITRATE, &bitrate);
    printf("speex_decoder_ctl:bitrate:%ld\n", bitrate);
    int32_t submode_encoding;
    speex_decoder_ctl(decoder_state, SPEEX_GET_SUBMODE_ENCODING, &submode_encoding);
    printf("speex_decoder_ctl:submode_encoding:%ld\n", submode_encoding);
    int32_t mode;
    speex_decoder_ctl(decoder_state, SPEEX_GET_MODE, &mode);
    printf("speex_decoder_ctl:mode:%ld\n", mode);

    fin = fopen(fin_name, "r");
    if (!fin)
    {
        printf("Could not open %s\n", fin_name);
        while (1) {}
    }
    printf("sample_size:%d\n", sample_size);
    fout = fopen(fout_name, "w");
    wav_header_t wav_header = wav_HEADER_PCM_DEFAULT(sample_size, 16, 8000, 1);
    fwrite(&wav_header, 1, sizeof(wav_header), fout);

    // Initialization of the structure that holds the bits
    SpeexBits bits;
    //printf("speex_bits_init()\n");
    speex_bits_init(&bits);

    int nbytes = speex_bits_nbytes(&bits);
    //printf("speex_bits_nbytes:nbytes:%d\n", nbytes);

    while (!feof(fin))
    {
        // Read data from file
        size_t pkg_size = fread(buf, 1, enc_frame_size, fin);
        if (pkg_size != enc_frame_size) {
            if (pkg_size)
                printf("speex_decode_: pkg_size != enc_frame_size: %d != %d\n", pkg_size, enc_frame_size);
            break;
        }

        //TickType_t tick1 = millis();

        // Copy the data into the bit-stream struct
        speex_bits_read_from(&bits, buf, pkg_size);
        // Decode the data
        speex_decode_int(decoder_state, &bits, out);

        //TickType_t tick2 = millis();
        //printf("speex_decode:tick2 - tick1:%ld\n", tick2 - tick1);

        // write to file
        fwrite(out, 2, FRAME_SIZE, fout);

        if (feof(fin))
            break;
    }
    // Destroy the decoder state
    //printf("speex_decoder_destroy()\n");
    speex_decoder_destroy(decoder_state);
    // Destroy the bit-stream struct
    //printf("speex_bits_destroy()\n");
    speex_bits_destroy(&bits);

    fclose(fout);
    fclose(fin);

    printf("SPEEX: Done decode\n");
}

void speex_encode_decode(const char *name, uint32_t quality) {
    size_t sample_size = 0;
    size_t enc_frame_size = 0;
    char src_name[100];
    char spx_name[100];
    char wav_name[100];
    printf("\n");
    sprintf(src_name, "%s.wav", name);
    sprintf(spx_name, "%s_%ld.spx", name, quality);
    sprintf(wav_name, "%s_%ld.wav", name, quality);
    speex_encode_(src_name, spx_name, quality, &sample_size, &enc_frame_size);
    speex_decode_(spx_name, wav_name, sample_size, enc_frame_size);
}

void speex_encode_decode_task()
{
    for (uint32_t quality = 0; quality <= 10; quality++) {
        if ((quality == 3) || (quality == 5) || (quality == 7))
            continue;

        speex_encode_decode("/sdcard/male", quality);
        speex_encode_decode("/sdcard/femal", quality);

        speex_encode_decode("/sdcard/mmt1", quality);
        speex_encode_decode("/sdcard/test", quality);
        speex_encode_decode("/sdcard/rwpcm", quality);

        speex_encode_decode("/sdcard/us10", quality);
        speex_encode_decode("/sdcard/us30", quality);
        speex_encode_decode("/sdcard/uk20", quality);
        speex_encode_decode("/sdcard/uk27", quality);
        speex_encode_decode("/sdcard/uk51", quality);
    }

    printf("DONE\n");
    while (1) {}
}

void wav_to_raw(const char* fin_name, const char* fout_name)
{
    FILE *fin;
    size_t n;

    printf("%s to %s\n", fin_name, fout_name);
    fin = fopen(fin_name, "r");
    if (!fin)
    {
        printf("Could not open %s\n", fin_name);
        while (1) {}
    }

    fseek(fin, 0, SEEK_END);
    n = ftell(fin) - sizeof(wav_header_t);
    printf("sample_size:%d\n", n);
    fseek(fin, 0, SEEK_SET);

    wav_header_t wav_header;
    fread(&wav_header, 1, sizeof(wav_header), fin);
    print_wav_header(&wav_header);

    FILE *fout;
    uint8_t b;
    fout = fopen(fout_name, "w");
    while (!feof(fin))
    {
        n = fread(&b, 1, 1, fin);
        if (n)
            fwrite(&b, 1, 1, fout);
    }
    fclose(fout);
    fclose(fin);
    printf("Done\n");
}

void wav_to_raw_task()
{
    wav_to_raw("/sdcard/wb_male.wav", "/sdcard/wb_male.raw");
    wav_to_raw("/sdcard/male.wav", "/sdcard/male.raw");
    wav_to_raw("/sdcard/female.wav", "/sdcard/female.raw");
    wav_to_raw("/sdcard/test.wav", "/sdcard/test.raw");

    while (1) {}
}

void raw_to_wav(const char* fin_name, const char* fout_name)
{
    FILE *fin;
    FILE *fout;
    uint8_t b;
    size_t n;

    // WAV_HEADER_PCM_DEFAULT(wav_sample_size, wav_sample_bits, wav_sample_rate, wav_channel_num)
    wav_header_t wav_header = WAV_HEADER_PCM_DEFAULT(0, 16, 8000, 1);

    printf("%s to %s\n", fin_name, fout_name);
    fin = fopen(fin_name, "r");
    if (!fin)
    {
        printf("Could not open %s\n", fin_name);
        while (1) {}
    }

    fseek(fin, 0, SEEK_END);
    n = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    printf("sample_size:%d\n", n);
    wav_header = wav_HEADER_PCM_DEFAULT(n, 16, 8000, 1);

    fout = fopen(fout_name, "w");

    fwrite(&wav_header, 1, sizeof(wav_header), fout);

    while (!feof(fin))
    {
        n = fread(&b, 1, 1, fin);
        if (n)
            fwrite(&b, 1, 1, fout);
    }
    fclose(fout);
    fclose(fin);
    printf("Done\n");
}

void raw_to_wav_task()
{
    raw_to_wav("/sdcard/wb_male.raw", "/sdcard/wb_male_.wav");
    raw_to_wav("/sdcard/male.raw", "/sdcard/male_.wav");
    raw_to_wav("/sdcard/female.raw", "/sdcard/female_.wav");
    raw_to_wav("/sdcard/test.raw", "/sdcard/test_.wav");
    raw_to_wav("/sdcard/rawpcm.raw", "/sdcard/rawpcm_.wav");

    while (1) {}
}

void app_main()
{
    // init SD-card
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 20
    };
    sdmmc_card_t *card;
    ESP_ERROR_CHECK(esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card));
    sdmmc_card_print_info(stdout, card);

    #ifdef FIXED_POINT
    #warning FIXED_POINT
    printf("FIXED_POINT\n");
    #endif
    #ifdef FLOATING_POINT
    #warning FLOATING_POINT
    printf("FLOATING_POINT\n");
    #endif

    //xTaskCreate(wav_to_raw_task, "wav_to_raw_task", 4096 * 2, NULL, 5, NULL);
    //xTaskCreate(raw_to_wav_task, "raw_to_wav_task", 4096 * 2, NULL, 5, NULL);
    xTaskCreate(speex_encode_decode_task, "speex_encode_decode_task", 4096 * 2 * 2, NULL, 5, NULL);
}
