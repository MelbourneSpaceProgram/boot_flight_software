pipeline {

    agent none

    stages {
       stage('Static Analysis') {
            agent {
                label 'master'
            }
			environment {
				PR_ID = sh (
					script: 'if [ -z ${CHANGE_ID+x} ]; then echo "-1"; else echo "$CHANGE_ID"; fi',
					returnStdout: true
					).trim()
			}
            steps {
                sh 'if [ -d "checker_output" ]; then rm -Rf checker_output; fi'
                sh 'mkdir checker_output'
                // Check conditional branches for "#define A" only. This doesn't exist, which means we skip the poor-scaling of checking all possible
                // configurations, with the trade-off of weaker static analysis results.
                sh 'cppcheck -DA --enable=warning,performance,style,portability --inconclusive --quiet --xml --xml-version=2 . 2> ./checker_output/cppcheck.xml'
                step([$class: 'WarningsPublisher', canComputeNew: false, canResolveRelativePaths: false, defaultEncoding: '', excludePattern: '', healthy: '', includePattern: '', messagesPattern: '', parserConfigurations: [[parserName: 'CppLint', pattern: '**/checker_output/cpplint.txt']], unHealthy: ''])
                sh 'cppcheck-htmlreport --source-encoding="iso8859-1" --title="MSP" --source-dir=. --report-dir=./checker_output/ --file=./checker_output/cppcheck.xml' 
                publishHTML([allowMissing: false, alwaysLinkToLastBuild: false, keepAll: false, reportDir: './checker_output/', reportFiles: 'index.html', reportName: 'Static Analysis', reportTitles: ''])
                step([
                    $class: 'ViolationsToGitHubRecorder', 
                    config: [
                        gitHubUrl: 'https://api.github.com/', 
                        repositoryOwner: 'MelbourneSpaceProgram', 
                        repositoryName: 'boot_flight_software', 
                        pullRequestId: "${PR_ID}", 
                        createCommentWithAllSingleFileComments: false, 
                        createSingleFileComments: true, 
                        commentOnlyChangedContent: false,       
                        credentialsId: 'a2c805e2-79f1-4a00-9fa5-d8e144e50245',
                        minSeverity: 'INFO',
                        keepOldComments: false,
                        violationConfigs: [
                            [ pattern: '.*/checker_output/.*\\.xml$', parser: 'CPPCHECK', reporter: 'CPPCHECK' ]
                        ]
                    ]
                ])
            }
        }
    
        stage('Build GCC') { 
            agent {
                label 'AWS_Docker_Agent'
            }
			environment {
				docker_name = org.apache.commons.lang.RandomStringUtils.random(20, true, true)
			}
            steps {
                sh '''
                    tar cf CDH_software.tar.gz -C ${WORKSPACE} .
                    sudo docker run -td -e CCACHE_DIR=/ccache --mount source=ccache,target=/ccache --name ${docker_name} ccsv8_msp432e_gnu
                    sudo docker exec -t $docker_name rm -rf /root/ws
                    sudo docker exec -t $docker_name mkdir /root/ws
                    sudo docker exec -t $docker_name ccache -s
                    sudo docker cp ${WORKSPACE}/CDH_software.tar.gz $docker_name:/root/
		    		sudo docker exec -t $docker_name mkdir /root/boot_flight_software
                    sudo docker exec -t $docker_name tar -xf /root/CDH_software.tar.gz -C /root/boot_flight_software/
                    sudo docker exec -t $docker_name /opt/ti/ccsv8/eclipse/eclipse -noSplash -data /root/ws -application com.ti.ccstudio.apps.projectImport -ccs.location /root/boot_flight_software/
                    sudo docker exec -t $docker_name /opt/ti/ccsv8/eclipse/eclipse -noSplash -data /root/ws -application com.ti.ccstudio.apps.projectBuild -ccs.workspace -ccs.configuration "Debug"
                    sudo docker exec -t $docker_name ccache -s
		    '''
    	    }
            post {
                always {
		    warnings canComputeNew: false, canResolveRelativePaths: false, canRunOnFailed: true, categoriesPattern: '', consoleParsers: [[parserName: 'GNU Make + GNU C Compiler (gcc)']], defaultEncoding: '', excludePattern: '', healthy: '', includePattern: '', messagesPattern: '', unHealthy: ''
                    sh '''
                     sudo docker rm -f ${docker_name}
                    '''
                }
            }
        }
    }
}
