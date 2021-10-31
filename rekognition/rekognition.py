import boto3
import json

def compare_faces(key,bucket):
		rekognition=boto3.client('rekognition')
		response = rekognition.compare_faces(
		SimilarityThreshold=90,
		SourceImage={
			'S3Object': {
				'Bucket':bucket,
				'Name': 'images/Recognition/fauzanimage1.jpeg',
			},
		},
		TargetImage={
			'S3Object': {
				'Bucket': bucket,
				'Name': key,
			},
		},
		)

		return response['FaceMatches']


def detect_text(key, bucket):
		rekognition=boto3.client('rekognition')
		response=rekognition.detect_text(Image={'S3Object':{'Bucket':bucket,'Name':key}})
		return response['TextDetections']
	

def detect_faces(bucket,key, region="us-east-2"):
		rekognition = boto3.client("rekognition",region)
		response= rekognition.detect_faces(
			Image={"S3Object":{
							"Bucket": bucket,
							"Name": key,
					}
				
			},
			Attributes=['ALL'],
			)
		return response['FaceDetails']
		
def detect_labels(bucket, key, max_labels=10, min_confidence=90, region="us-east-2"):
		rekognition = boto3.client("rekognition", region)
		response = rekognition.detect_labels(
			   Image={"S3Object": {
							   "Bucket": bucket,
							   "Name": key,
					   }
			   },
			   MaxLabels=max_labels,
			   MinConfidence=min_confidence,
		)
		
		
		return response['Labels']

def lambda_handler(event, context):
	results = ''
	mqtt = boto3.client('iot-data', region_name='us-east-2')

	bucket_name = 'esp32cam-121525850774'
	file_name = str(event['payload'])


	for label in detect_labels(bucket_name, file_name):
		if (float(label['Confidence']) > 90):
				results += (label['Name'] + ';')
	
	for facedetail in detect_faces(bucket_name, file_name):
		if (float(facedetail['Confidence']) > 30):
			results += (str(facedetail['Gender']['Value']) + ';')

	for text in detect_text(file_name, bucket_name):
		print ('Detected text:' + text['DetectedText'])
		# print ('Confidence: ' + "{:.2f}".format(text['Confidence']) + "%")
		# print ('Id: {}'.format(text['Id']))
		# if 'ParentId' in text:
		#     print ('Parent Id: {}'.format(text['ParentId']))
		# print ('Type:' + text['Type'])
		# print()
		if (float(text['Confidence']) > 90):
			results += (text['DetectedText']+';')
	
	for faces in compare_faces(file_name, bucket_name):
		if (float(faces['Similarity']) > 90):
			results += (str(faces['Face']['Confidence'])+';')
			
	
	response = mqtt.publish(
			topic='esp32/sub/dataiot',
			qos=0,
			payload=results
		)