#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>

#include <sys/uio.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <semaphore.h>
#include <pthread.h>

#include <arv.h>
//#include "arvcamera/arvcamera.h"
#include "cvfilter/cvfilter.h"

#include "arvcamera/arvcollector.h"

#if 0
void *collect_data_thread(void *arg){

    struct data_collector *collector = (struct data_collector*)arg;
    int exit_flag = collector->exit_flag;

    cv::Mat img;
    img = cv::Mat::zeros(IMAGE_RAW_HEIGHT, IMAGE_RAW_WIDTH, CV_8UC3);
    int imgSize = img.total() * img.elemSize();

    //

    fprintf(stdout, "[collect_data_thread]: stop\n");
    fflush(stdout);
    return NULL;
}
#else

typedef struct {
    GMainLoop *main_loop;
    int buffer_count;
    struct data_collector *collector;
} ApplicationData;

static int cancel = 0;

static void set_cancel(int signal) {
    cancel = 1;
}

static void new_buffer_cb(ArvStream *stream, ApplicationData *data) {
    ArvBuffer *buffer = arv_stream_try_pop_buffer(stream);
    char name[256];
    static int id = 0;
    int frame_id = -1;
    if (buffer != NULL) {
        if (arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS) {
            data->buffer_count++;
            printf("%lu\n", arv_buffer_get_timestamp(buffer));
            size_t bytes;
            void *img_data = (void*)arv_buffer_get_data(buffer, &bytes);
            cv::Mat img(arv_buffer_get_image_height(buffer), arv_buffer_get_image_width(buffer), CV_8UC1, img_data);
//------------------------------------------------------------------------------------
                float radius = 0; //findRadius(img);
#if 0
                int frames_obj_cnt = 0;
                int frames_empty_cnt = 0;
                int frames_non_cnt = 0;
                int new_object = 0;

                //not smart algotithm of object detection
                //fprintf(stdout, "[tcp_capture_data_process]: frames_obj_cnt = %d frames_empty_cnt %d frames_non_cnt %d\n", frames_obj_cnt, frames_empty_cnt, frames_non_cnt);
                if (radius < 135 && new_object == 0) {
                    frames_empty_cnt++;
                    if (frames_obj_cnt >= 16 && frames_empty_cnt >= 8) {
                        new_object = 1;
                        fprintf(stdout, "[tcp_capture_data_process]: object break is detected after %d frames\n", frames_empty_cnt + frames_obj_cnt + frames_non_cnt);
                        //finalize current full object
                        sem_post(&collector->data_ready);
                        //prepare new empty object
                        new_object = 0;
                        frames_obj_cnt = 0; 
                        frames_non_cnt = 0;
                        frames_empty_cnt = 0;
                    }
                }
                else if (radius >= 150){
                   frames_obj_cnt++;
                   frames_obj_cnt += frames_empty_cnt;
                   frames_empty_cnt = 0;
                   frames_obj_cnt += frames_non_cnt;
                   frames_non_cnt = 0;
                }
                else {
                    frames_non_cnt++;
                }
#else
    sem_post(&(data->collector)->data_ready);
#endif
//-------------------------------------------------------------------------------------
            sprintf(name, "../data/test_%d.bmp", id++);
            cv::imwrite(name, img);
            fprintf(stdout, "Frame %d with frame_id %d is ready, radius = %f \n", id++, frame_id, radius);
            fflush(stdout);
            arv_stream_push_buffer(stream, buffer);
        }
    }
}

static gboolean periodic_task_cb (void *abstract_data) {
    ApplicationData *data = (ApplicationData *)abstract_data;

    fprintf(stdout, "Frame rate = %d Hz\n", data->buffer_count);
    data->buffer_count = 0;

    if (cancel) {
        g_main_loop_quit(data->main_loop);
        return 0;
    }

    return 1;
}

static void control_lost_cb (ArvGvDevice *gv_device) {
    // Control of the device is lost. Display a message and force application exit
    fprintf (stdout, "Control lost\n");
    fflush(stdout);
    cancel = 1;
}

struct data_collector *collector_init(char* source) {

    struct data_collector *collector = (struct data_collector *)malloc(sizeof(struct data_collector));
    if (!collector) {
        fprintf(stderr, "[collector_init]: can't allocte collector\n");
        fflush(stderr);
        return NULL;
    }

    strcpy(collector->source, source);
    collector->exit_flag = 0;
    
    if (sem_init(&(collector->data_ready), 1, 0) == -1) {
        fprintf(stderr, "[cvcollector]: can't init object collector semaphore\n");
        return NULL;
    }

    ApplicationData data;
    ArvCamera *camera;
    ArvStream *stream;
    GError *error = NULL;

    data.buffer_count = 0;
    //TODO: ApplicationData and struct collector are here by the same reasons --- (void*)abstract_data for new thread
    data.collector = collector;

    // Instantiation of the source camera
    camera = arv_camera_new(source, &error);

    if (ARV_IS_CAMERA (camera)) {
        void (*old_sigint_handler)(int);
        gint payload;

        // Set region of interrest to a 200x200 pixel area
        arv_camera_set_region(camera, 0, 0, 200, 200, NULL);
        // Set frame rate to 16 Hz
        arv_camera_set_frame_rate(camera, 16.0, NULL);
        // retrieve image payload (number of bytes per image)
        payload = arv_camera_get_payload (camera, NULL);

        // Create a new stream object
        stream = arv_camera_create_stream(camera, NULL, NULL, &error);
        if (ARV_IS_STREAM (stream)) {
            // Push 50 buffer in the stream input buffer queue
            for (int i = 0; i < 50; i++)
                arv_stream_push_buffer(stream, arv_buffer_new(payload, NULL));

            // Start the video stream
            arv_camera_start_acquisition(camera, NULL);

            // Connect the new-buffer signal
            g_signal_connect(stream, "new-buffer", G_CALLBACK(new_buffer_cb), &data);
            // And enable emission of this signal (it's disabled by default for performance reason)
            arv_stream_set_emit_signals(stream, TRUE);

			/* Connect the control-lost signal */
			g_signal_connect(arv_camera_get_device(camera), "control-lost",
					  G_CALLBACK(control_lost_cb), NULL);

			/* Install the callback for frame rate display */
			g_timeout_add_seconds(1, periodic_task_cb, &data);

			/* Create a new glib main loop */
			data.main_loop = g_main_loop_new(NULL, FALSE);

			old_sigint_handler = signal(SIGINT, set_cancel);

			/* Run the main loop */
			g_main_loop_run(data.main_loop);

			signal(SIGINT, old_sigint_handler);

			g_main_loop_unref(data.main_loop);

			/* Stop the video stream */
			arv_camera_stop_acquisition(camera, NULL);

			/* Signal must be inhibited to avoid stream thread running after the last unref */
			arv_stream_set_emit_signals(stream, FALSE);

			g_object_unref(stream);
		}
		else {
			fprintf(stdout, "Can't create stream thread%s%s\n",
				error != NULL ? ": " : "",
				error != NULL ? error->message : "");
            fflush(stdout);
            g_clear_error (&error);
        }
        g_object_unref (camera);
    } 
    else {
        fprintf(stdout, "No camera found%s%s\n", error != NULL ? ": " : "", error != NULL ? error->message : "");
        fflush(stdout);
        g_clear_error (&error);
    }
    fprintf(stdout, "[collect_data_thread]: stop\n");
    return collector;
}
#endif

void collector_destroy(struct data_collector *collector){
    collector->exit_flag = 1;
    //TODO: when sem_wait???
    free(collector);
}

