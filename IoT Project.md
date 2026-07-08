**COMP6733 IoT Design Studio — Project Proposal**

**A7: Mini Germination Chamber**

*A Low-Cost IoT System for Monitoring Seed Germination*

# **Topic Definition**

## **Purpose**

This project aims to design, build, and evaluate a low-cost IoT system that continuously monitors seed germination conditions and automatically detects germination using computer vision. By combining environmental telemetry (temperature, humidity, light, and substrate moisture) with image-based germination detection, the system links germination outcomes to the exact conditions under which they occurred.

## **In Scope**

* A ventilated, compact enclosure housing seed trays, sensors, and two fixed camera nodes.

* Sensors for air and soil temperature, humidity, soil moisture, and light.

* A Raspberry Pi hub performing scheduled sampling, local buffering, MQTT publishing, broker hosting, and on-device model inference.

* Two ESP32-CAM nodes capturing fixed-frame tray images and exposing them over HTTP for the Raspberry Pi to collect.

* A tested image-classification model that scores each captured frame and outputs germinated / not-germinated counts.

* MQTT publishing of sensor data, images, and detection counts to a Home Assistant server providing dashboards, time-lapse, alerting, and logging.

* An evaluation harness measuring sensing accuracy, reliability, energy use, and detection/germination performance, with model output validated against manual checks.

## **Out of scope**

* Active environmental control: the system does not automate heating, cooling, forced airflow, humidification, or programmable lighting actuators. Conditions are only logged and reported.

* Any combustion/smoke-generation/water-related hardware.

* Full grow-to-harvest cultivation, nutrient dosing. The system only targets the germination phase.

* Pest/disease management beyond basic hygiene and ventilation.

* Reproduction of contrasting species regimes. The trial centres on a single fast-germinating species (mung bean).

## **Contribution and motivation**

Manual germination checks are limited because infrequent, subjective observations can miss the exact time of germination. Additionally, conventional dataloggers record environmental conditions but not germination outcomes. This project addresses both limitations with a low-cost IoT system that continuously monitors temperature, humidity, light, and substrate moisture, and automatically detects germination using computer vision. Linking each germination event to the environmental conditions that produced it.

# **Background**

## **Germination and environmental sensitivity**

Germination depends on environmental factors such as temperature, moisture, light, and gas exchange, with most species exhibiting a narrow optimal temperature range. Mung bean is an ideal demonstrator because it germinates rapidly (2-5 days) while it is sensitive to temperature, achieving 85-100% germination at 25-30 °C but dropping to around 30% under colder conditions.

Automated vision-based germination counting is a proven, reliable alternative to manual scoring, with strong results from computer vision and deep learning across multiple species. This project extends this approach by pairing image-based germination detection with continuous environmental logging. Thus, linking each germination event to its associated conditions.

## **Sensing**

Air temperature, humidity and light are measured using the given Arduinos. Substrate moisture is monitored with an Adafruit STEMMA capacitive soil-moisture sensor over I2C to reduce corrosion and drift associated with resistive probes. Overall, sensors are selected for calibrated digital output and stability to support unattended multi-day operation.

## **Image-based germination detection**

Two ESP32-CAM nodes capture fixed-frame images of the planted seeds from repeatable tray angles. The Raspberry Pi periodically pulls each camera's HTTP image endpoint, stores timestamped frames, and later processes them with a lightweight classifier that outputs binary germinated vs non-germinated counts. Capture frequency is balanced against storage and bandwidth constraints. Model outputs are validated against manual ground-truth counts on a held-out dataset.

## **Connectivity, monitoring, and the Home Assistant platform**

The Raspberry Pi handles scheduled sampling, image capture, local buffering, sensor data publishing, images, and detection outputs via an MQTT broker running on-device. Home Assistant subscribes to these topics to provide dashboards, time-lapse views, and historical logging. Consolidating the broker, Home Assistant, and inference on a single Pi simplifies the architecture, though it increases dependency on a single point of failure.

# **Project Plan**

## **Milestones and schedule**

The build is organised into ten milestones across the term. Table 1 sets out the duration and key resources for each.

| Milestone | Key activities | Weeks |
| :---- | :---- | :---- |
| M1 Requirements & design | Finalise functional/non-functional requirements and system block design for the monitoring architecture. | 1–4 |
| M2 Procurement (BOM) | Order materials. | 1–4 |
| M3 Enclosure & mounting | Build the ventilated enclosure; mount the tray, sensors, and camera. Manage cables. | 4–5 |
| M4 Sensor integration & logging | Wire and read all sensors. Baseline checks of sensor data. Local logging on the Pi. | 4–6 |
| M5 Image capture pipeline | Fixed-framing scheduled capture from ESP32-CAM HTTP endpoints. Storage management. | 5–7 |
| M6 MQTT \+ HA dashboards | Mosquitto broker and Home Assistant dashboards on the Pi. Alerting and historical logging. | 6–8 |
| M7 Detection model (train/validate) | Train classification model. Validate against manual ground-truth counts. | 7–9 |
| M8 Integration & tuning | End-to-end integration. Reliability and recovery testing under induced Wi-Fi and power loss. | 7–9 |
| M9 Monitoring trial run | Mung bean trial across replicate trays. Collect sensor, image, and detection data. | 8–9 |
| M10 Evaluation & write-up | Analyse results against the four evaluation criteria. Final report, demo, and presentation. | 9–10 |

*Table 1\. Milestones.*

**By Week 8,** we will have completed milestones M1-M6. **Our final deliverable** will be a fully working, tested monitoring enclosure that reads sensor data, produces visual insights, and classifies germinated/non-germinated seeds. Thus, allowing users to find the best environment for mung-bean seed germination.

## **Bill of materials (BOM)**

Order-of-magnitude costs and part selections are for planning purposes; confirm against current supplier pricing before final purchase.

| Component / role | Example part |
| :---- | :---- |
| Hub / controller | Raspberry Pi  |
| Camera | 2 x ESP32-CAM modules |
| Soil moisture | Adafruit STEMMA capacitive sensor (I2C) |
| Light level, Temperature/Humidity sensing | Arduino sensors |
| I2C cable & jumper wires | STEMMA QT adapter |
| Power supply | USB-C 5V/3A |
| Boot drive | MicroSD card (32GB, A1/A2) |
| Enclosure & mounting | Ventilated plastic box, cable glands, wiring |
| Wiring & prototyping | Housing, camera mount, cable glands, wiring |

*Table 2\. Bill of materials.*

## **Risk assessment**

The risk register below covers the principal technical and biological risks to the project, with likelihood, impact, and mitigation for each.

| Risk | Likelihood/impact | Mitigation |
| :---- | :---- | :---- |
| Image recognition misclassifies germination status | Medium / Medium | Validate model output against manual ground-truth counts on a held-out sample. Fix camera angle and lighting to cut variability. |
| Condensation affects camera lens or electronics | Medium / High | Keep camera and electronics clear of direct moisture. Use protective housing and a lens shield. |
| Seed-to-seed / tray-to-tray variability | Medium / Medium | Mung bean's fast 2-5 day cycle allows multiple replicate runs within the window. Use one seed lot and average across trays. |
| Wi-Fi / MQTT dropout loses data | Medium / Medium | Sensor data and images are stored locally on the Pi's SD card, independent of the network. Home Assistant and Mosquitto auto-reconnect on restart. |
| Component lead times delay the build | Medium / Medium | Keep the critical path (Pi, camera, sensors) short. |

*Table 3\. Risk register.*

## **Evaluation methodology**

* Sensing accuracy. Compare multiple sensor readings.

* Reliability. System uptime, MQTT message delivery/loss rate, sensor-dropout frequency, and recovery after power interruptions.

* Power draw. Total energy drawn by the sensors, Raspberry Pi, camera, and server over a multi-day run.

* Detection performance. Precision and recall of the image classifier against manual ground-truth counts, with germination rate and time-to-germination derived from the validated detection counts. 

# **References**

* Adafruit Industries (2026) *STEMMA soil sensor — I2C capacitive moisture sensor*. Available at: [https://www.adafruit.com/product/4026](https://www.adafruit.com/product/4026) (Accessed: 19 June 2026).  
* Australian Native Plants Society (Australia) (no date) *Smoke germination of Australian plants*. Available at: [https://anpsa.org.au/smoke-germination-of-australian-plants/](https://anpsa.org.au/smoke-germination-of-australian-plants/) (Accessed: 24 June 2026).  
* Eclipse Foundation (2026) *Eclipse Mosquitto: an open source MQTT broker*. Available at: [https://mosquitto.org/](https://mosquitto.org/) (Accessed: 22 June 2026).  
* ElMasry, G., Mandour, N., Wagner, M-H., Demilly, D., Verdier, J., Belin, E. and Rousseau, D. (2019) 'Utilization of computer vision and multispectral imaging techniques for classification of cowpea (*Vigna unguiculata*) seeds', *Plant Methods*, 15, Article 24\. Available at: [https://doi.org/10.1186/s13007-019-0411-2](https://doi.org/10.1186/s13007-019-0411-2) (Accessed: 23 June 2026).  
* Home Assistant (2026) *Home Assistant documentation*. Available at: [https://www.home-assistant.io/docs/](https://www.home-assistant.io/docs/) (Accessed: 27 June 2026).  
* Kaur, K., Goyal, R., Shekhar, S., Nath, A. and Prasad, K. (2026) 'Non-destructive machine vision system for quality assessment of mung bean (*Vigna radiata*)', *Food Analytical Methods*, 19, Article 216\. Available at: [https://doi.org/10.1007/s12161-026-03132-6](https://doi.org/10.1007/s12161-026-03132-6) (Accessed: 21 June 2026).  
* Masteling, R., Voorhoeve, L., IJsselmuiden, J., Dini-Andreote, F., de Boer, W. and Raaijmakers, J.M. (2020) 'DiSCount: computer vision for automated quantification of *Striga* seed germination', *Plant Methods*, 16, Article 60\. Available at: [https://doi.org/10.1186/s13007-020-00602-8](https://doi.org/10.1186/s13007-020-00602-8) (Accessed: 20 June 2026).  
* OASIS (2019) *MQTT version 5.0 specification*. Available at: [https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html) (Accessed: 18 June 2026).  
* Raspberry Pi Ltd (2024) *Raspberry Pi 4 Model B*. Available at: [https://www.raspberrypi.com/products/raspberry-pi-4-model-b/](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/) (Accessed: 29 June 2026).  
* ROHM Semiconductor (2023) *BH1750FVI-TR: digital 16-bit serial output type ambient light sensor IC — datasheet*. Available at: [https://www.rohm.com/products/sensors-mems/ambient-light-sensor-ics/bh1750fvi-tr](https://www.rohm.com/products/sensors-mems/ambient-light-sensor-ics/bh1750fvi-tr) (Accessed: 26 June 2026).  
* Škrubej, U., Rozman, Č. and Stajnko, D. (2015) 'Assessment of germination rate of the tomato seeds using image processing and machine learning', *European Journal of Horticultural Science*, 80(2), pp. 68–75. Available at: [https://doi.org/10.17660/eJHS.2015/80.2.4](https://doi.org/10.17660/eJHS.2015/80.2.4) (Accessed: 23 June 2026).
